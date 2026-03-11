#!/usr/bin/env python3
"""batch-gemini.py (-f prompt-file | -c prompt) [-H|-L|-X|-0] [-n N] [-w seconds] [-m model] < stdin > stdout

Read texts from stdin, take N at a time (default: 100), append them to the
prompt-file contents, call Gemini flash-lite, and print the response text.
  -H  heads mode (default): docs separated by ==> FILENAME <==
  -L  lines mode: each line is a separate doc
  -X  xml mode: docs separated by </DOC>
  -0  null mode: docs separated by \0
"""

import sys, os, re, json, time, argparse, urllib.request

def call_gemini(prompt, api_key, model="gemini-2.5-flash-lite"):
    """Send prompt to Gemini and return the response text."""
    url = (
        "https://generativelanguage.googleapis.com/v1beta/models/"
        f"{model}:generateContent"
    )
    payload = {
        "contents": [
            {"role": "user", "parts": [{"text": prompt}]}
        ],
        "generationConfig": {
            "maxOutputTokens": 65536,
            "temperature": 0,
            "candidateCount": 1,
        },
        "safetySettings": [
            {"category": "HARM_CATEGORY_HATE_SPEECH",       "threshold": "BLOCK_NONE"},
            {"category": "HARM_CATEGORY_HARASSMENT",        "threshold": "BLOCK_NONE"},
            {"category": "HARM_CATEGORY_SEXUALLY_EXPLICIT", "threshold": "BLOCK_NONE"},
            {"category": "HARM_CATEGORY_DANGEROUS_CONTENT", "threshold": "BLOCK_NONE"},
        ],
    }
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        headers={
            "Content-Type": "application/json",
            "X-goog-api-key": api_key,
        },
        method="POST",
    )
    with urllib.request.urlopen(req) as resp:
        result = json.loads(resp.read())
    # concatenate all text parts (same as jq in gg)
    parts = result.get("candidates", [{}])[0].get("content", {}).get("parts", [])
    return "".join(p.get("text", "") for p in parts)

def read_heads():
    """Yield docs from stdin, each including its ==> FILENAME <== header."""
    lines = []
    for line in sys.stdin:
        if re.match(r'^==> .+? <==$', line):
            if lines:
                yield "".join(lines)
            lines = [line]
        else:
            lines.append(line)
    if lines:
        yield "".join(lines)

def read_lines():
    """Yield lines from stdin, one at a time."""
    for line in sys.stdin:
        yield line

def read_xml():
    """Yield docs from stdin, split by </DOC>."""
    lines = []
    for line in sys.stdin:
        if line.strip() == '</DOC>':
            yield "".join(lines)
            lines = []
        else:
            lines.append(line)
    if lines:
        yield "".join(lines)

def read_null():
    """Yield docs from stdin, split by null bytes."""
    data = sys.stdin.buffer.read()
    for part in data.split(b'\0'):
        text = part.decode('utf-8', errors='replace')
        if text:
            yield text

def process_batch(batch, prompt_template, api_key, model):
    """Build prompt from batch of texts, call Gemini, write response."""
    prompt = prompt_template + "".join(batch)
    try:
        response = call_gemini(prompt, api_key, model)
        sys.stdout.write(response)
        if not response.endswith("\n"):
            sys.stdout.write("\n")
        sys.stdout.flush()
    except urllib.error.URLError as e:
        time.sleep(5)

def process_stream_of_texts(docs, prompt_template, api_key, batch_size, wait, model="gemini-2.5-flash-lite"):
    """Consume doc generator in batches, call Gemini for each batch."""
    batch = []
    total = 0
    for doc in docs:
        batch.append(doc)
        if len(batch) >= batch_size:
            t0 = time.time()
            process_batch(batch, prompt_template, api_key, model)
            total += len(batch)
            batch = []
            time.sleep(wait)
            print(f"{time.time()-t0:.1f}s\t{total} texts\t", file=sys.stderr)
    if batch:
        process_batch(batch, prompt_template, api_key, model)

def main():
    parser = argparse.ArgumentParser(
        description="Batch-call Gemini flash-lite on texts from stdin.")
    prompt = parser.add_mutually_exclusive_group(required=True)
    prompt.add_argument('-f', metavar='FILE', help='read prompt from file')
    prompt.add_argument('-c', metavar='PROMPT', help='prompt string')
    parser.add_argument('-n', type=int, default=100, metavar='N', help='texts per batch (100)')
    parser.add_argument('-w', type=float, default=0, metavar='SEC', help='sleep after calls (0)')
    parser.add_argument('-H', action='store_true', default=True, help='heads mode: ==> FILE <== (default)')
    parser.add_argument('-L', action='store_true', help='lines mode: one doc per line')
    parser.add_argument('-X', action='store_true', help='xml mode: docs separated by </DOC>')
    parser.add_argument('-0', action='store_true', dest='null', help='null mode: docs separated by \\0')
    parser.add_argument('-m', default='gemini-2.5-flash-lite', metavar='MODEL', help='model (gemini-2.5-flash-lite)')
    opts = parser.parse_args()
    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        print("Error: GEMINI_API_KEY not set", file=sys.stderr)
        sys.exit(1)
    if opts.f:
        if not os.path.isfile(opts.f):
            print(f"Error: prompt file not found: {opts.f}", file=sys.stderr)
            sys.exit(1)
        with open(opts.f) as fh:
            prompt_template = fh.read()
    else:
        prompt_template = opts.c + "\n"
    print(f"Applying prompt to ==> texts <==:\n{prompt_template}", file=sys.stderr)
    docs = read_lines() if opts.L else read_xml() if opts.X else read_null() if opts.null else read_heads()
    process_stream_of_texts(docs, prompt_template, api_key, opts.n, opts.w, opts.m)
if __name__ == "__main__":
    main()
