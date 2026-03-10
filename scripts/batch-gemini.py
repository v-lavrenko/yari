#!/usr/bin/env python3
"""batch-gemini.py -p prompt-file [-nf N] [-w seconds] [-m model] < stdin > stdout

Read texts delimited by '==> FILENAME <==' from stdin.
Take N texts at a time (default: 100), append them to the prompt-file contents,
call Gemini flash-lite, and print the concatenated response text.
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

def read_docs():
    """Yield (filename, text) tuples from stdin, one at a time."""
    filename = None
    lines = []
    header_re = re.compile(r'^==> (.+?) <==$')
    for line in sys.stdin:
        m = header_re.match(line)
        if m:
            if filename is not None:
                yield (filename, "".join(lines))
            filename = m.group(1).strip()
            lines = []
        else:
            lines.append(line)
    if filename is not None:
        yield (filename, "".join(lines))

def process_batch(batch, prompt_template, api_key, model):
    """Build prompt from batch of docs, call Gemini, write response."""
    body = ""
    for filename, text in batch:
        body += f"\n==> {filename} <==\n{text}"
    prompt = prompt_template + body
    try:
        response = call_gemini(prompt, api_key, model)
        sys.stdout.write(response)
        if not response.endswith("\n"):
            sys.stdout.write("\n")
    except urllib.error.URLError as e:
        time.sleep(5)

def process_stream_of_texts(prompt_template, api_key, batch_size, wait, model="gemini-2.5-flash-lite"):
    """Read docs from stdin, batch them, and call Gemini for each batch."""
    batch = []
    total = 0
    for doc in read_docs():
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
        description="Batch-call Gemini flash-lite on ==> FILENAME <== delimited texts from stdin.")
    parser.add_argument('-p', required=True, metavar='FILE', help='prompt file')
    parser.add_argument('-nf', type=int, default=100, metavar='N', help='texts per batch (100)')
    parser.add_argument('-w', type=float, default=0, metavar='SEC', help='sleep after calls (0)')
    parser.add_argument('-m', default='gemini-2.5-flash-lite', metavar='MODEL', help='model (gemini-2.5-flash-lite)')
    opts = parser.parse_args()
    prompt_file = opts.p
    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        print("Error: GEMINI_API_KEY not set", file=sys.stderr)
        sys.exit(1)
    if not os.path.isfile(prompt_file):
        print(f"Error: prompt file not found: {prompt_file}", file=sys.stderr)
        sys.exit(1)
    with open(prompt_file) as f:
        prompt_template = f.read()
    print(f"Applying prompt to ==> texts <==:\n{prompt_template}", file=sys.stderr)
    process_stream_of_texts(prompt_template, api_key, opts.nf, opts.w, opts.m)
if __name__ == "__main__":
    main()
