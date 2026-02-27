/*

  Copyright (c) 1997-2024 Victor Lavrenko (v.lavrenko@gmail.com)

  This file is part of YARI.

  YARI is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  YARI is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU General Public License
  along with YARI. If not, see <http://www.gnu.org/licenses/>.

*/

#include "curl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gemini.h"
#include "hash.h"
#include "textutil.h"
#include "synq.h"
#include "coll.h"
#include "matrix.h"

// -------------------- text_from_response --------------------

// Extract and concatenate all "text" parts from a Gemini API response.
// response: {"candidates":[{"content":{"parts":[{"text":"..."},...]}}]}
// Returns malloc'd string or NULL. Caller frees.
char *text_from_response (char *response) {
  char *parts = json_value (response, "parts");
  char *result = NULL, *pos = NULL, *search = parts;
  int result_sz = 0;
  while (search && (pos = strstr (search, "\"text\""))) {
    char *part = json_value (pos, "text");
    if (!part) break;
    json_unescape (part);
    zcat (&result, &result_sz, part);
    free (part);
    search = pos + 6;
  }
  free (parts);
  return result;
}

// -------------------- generate_text --------------------

// Send a prompt to the Gemini API and return the generated text.
// model: e.g. "gemini-2.5-flash-lite". Returns malloc'd string. Caller frees.
char *generate_text (char *prompt, char *model) {
  if (!prompt || !*prompt) return NULL;
  if (!model || !*model) model = "gemini-2.5-flash-lite";
  char *api_key = getenv ("GEMINI_API_KEY");
  if (!api_key) assert (0 && "[generate_text] GEMINI_API_KEY not set\n");

  char *escaped = json_escape (prompt);
  if (!escaped) return NULL;

  // build JSON payload (matches scripts/gg format)
  size_t pay_sz = strlen(escaped) + 1024;
  char *payload = malloc (pay_sz);
  snprintf (payload, pay_sz,
    "{\"contents\":[{\"role\":\"user\",\"parts\":[{\"text\":\"%s\"}]}],"
    "\"generationConfig\":{\"maxOutputTokens\":65536,\"temperature\":0.5,"
    "\"topK\":0,\"topP\":0.95,\"candidateCount\":1},"
    "\"safetySettings\":["
    "{\"category\":\"HARM_CATEGORY_HATE_SPEECH\",\"threshold\":\"BLOCK_NONE\"},"
    "{\"category\":\"HARM_CATEGORY_HARASSMENT\",\"threshold\":\"BLOCK_NONE\"},"
    "{\"category\":\"HARM_CATEGORY_SEXUALLY_EXPLICIT\",\"threshold\":\"BLOCK_NONE\"},"
    "{\"category\":\"HARM_CATEGORY_DANGEROUS_CONTENT\",\"threshold\":\"BLOCK_NONE\"}"
    "]}", escaped);
  free (escaped);

  // build URL
  char url[512];
  snprintf (url, sizeof(url),
    "https://generativelanguage.googleapis.com/v1beta/models/"
    "%s:generateContent", model);

  // use X-goog-api-key header (like scripts/gg)
  char auth[256];
  snprintf (auth, sizeof(auth), "X-goog-api-key: %s", api_key);
  char *hdrs[] = {"Content-Type: application/json", auth, NULL};

  char *response = curl (url, hdrs, payload);
  free (payload);

  if (!response) {
    fprintf (stderr, "[generate_text] no response\n");
    return NULL;
  }

  // check for API error
  char *err = json_value (response, "message");
  if (err) {
    fprintf (stderr, "[generate_text] API error: %s\n", err);
    free (err);
    free (response);
    return NULL;
  }

  // extract generated text from API response
  char *result = text_from_response (response);
  free (response);
  if (!result) fprintf (stderr, "[generate_text] no text in response\n");
  return result; // caller frees
}

// -------------------- generate_texts --------------------



static void *_generate (void *prompt) {
  return generate_text ((char *)prompt, "gemini-2.5-flash-lite");
}

// Call generate_text on each prompt in parallel using nt threads.
// Returns a vector, like prompts. NULLs indicate failures.
char **generate_texts (uint nt, char **prompts) {
  uint n = len(prompts);
  char **texts = new_vec (n, sizeof (char*));
  parallel (nt, _generate, (void **)prompts, (void **)texts, n);
  return texts;
}

// -------------------- embedding_from_response --------------------

// Extract embedding vector from a Gemini API response.
// response: {"embedding": {"values": [0.1, -0.2, ...]}}
// Returns new_vec of floats or NULL. Caller frees.
float *embedding_from_response (char *response) {
  char *vals = json_value (response, "values");
  if (!vals) {
    char *err = json_value (response, "message");
    fprintf (stderr, "[embed] API error: %s\n", err ? err : response);
    free (err);
    return NULL;
  }
  float *vec = json_list_of_floats (vals);
  free (vals);
  return vec;
}

// -------------------- embed_text --------------------

float *embed_text (char *text) {
  if (!text || !*text) return NULL;
  char *api_key = getenv ("GEMINI_API_KEY");
  if (!api_key) assert (0 && "[embed_text] GEMINI_API_KEY not set\n");

  // JSON-escape the input text
  char *escaped = json_escape (text);
  if (!escaped) return NULL;

  // build the JSON payload
  size_t pay_sz = strlen(escaped) + 256;
  char *payload = malloc (pay_sz);
  snprintf (payload, pay_sz,
    "{\"model\":\"models/gemini-embedding-001\","
    "\"content\":{\"parts\":[{\"text\":\"%s\"}]}}", escaped);
  free (escaped);

  // build the URL
  char url[512];
  snprintf (url, sizeof(url),
    "https://generativelanguage.googleapis.com/v1beta/models/"
    "gemini-embedding-001:embedContent?key=%s", api_key);

  char *hdrs[] = {"Content-Type: application/json", NULL};
  char *response = curl (url, hdrs, payload);
  free (payload);

  if (!response) {
    fprintf (stderr, "[embed_text] no response\n");
    return NULL;
  }

  float *vec = embedding_from_response (response);
  free (response);
  return vec; // caller uses len(vec) and free_vec(vec)
}

// Embed each text in parallel using nt threads.
// Returns a vector of float* vectors. NULLs indicate failures.

static void *_embed (void *text) {
  return embed_text ((char *)text);
}

float **embed_texts (uint nt, char **texts) {
  uint n = len(texts);
  float **vecs = new_vec (n, sizeof (float*));
  parallel (nt, _embed, (void **)texts, (void **)vecs, n);
  return vecs;
}

// -------------------- embed_coll --------------------

void embed_coll (char *_texts, char *_vecs, char *prm) {
  uint batch = getprm(prm, "batch=", 10);
  uint threads = getprm(prm, "threads=", 1);
  coll_t *TEXTS = open_coll (_texts, "r+");
  coll_t *VECS  = open_coll (_vecs, "w+");
  uint N = nvecs(TEXTS);
  void **texts = calloc (batch, sizeof (void*));
  void **vecs  = calloc (batch, sizeof (void*));
  fprintf (stderr, "embed_coll: %s[%d] -> %s\n", _texts, N, _vecs);
  for (uint i = 0; i < N; i += batch) {
    uint n = (i + batch < N) ? batch : (N - i); // number of texts in this batch  
    for (uint j = 0; j < n; ++j) {
      char *txt = get_chunk (TEXTS, i+j+1);
      texts[j] = txt ? strdup(txt) : NULL;
    }
    parallel (threads, _embed, texts, vecs, n);
    for (uint j = 0; j < n; ++j) {
      ix_t *vec = full2vec(vecs[j]);
      if (vec) put_vec (VECS, i+j+1, vec);
      free_vec(vecs[j]);
      free_vec(vec);
      free(texts[j]);
    }
    show_progress (N, i+n, " texts embedded");
  }
  fprintf (stderr, "done: %s[%d]\n", _vecs, nvecs(VECS));
  free (texts);
  free (vecs);
  free_coll (TEXTS);
  free_coll (VECS);
}

// -------------------- main (test) --------------------

#ifdef MAIN

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

char *usage =
  "gemini -embed \"text\"                 ... print embedding vector\n"
  "gemini -embed-file file.txt            ... embed contents of file\n"
  "gemini -gen [-0|-1|-9] \"prompt\"      ... generate text\n"
  "gemini VECS = embed:prm KVS            ... embed a collection\n"
  ;

void do_embed (char *text) {
  float *vec = embed_text (text);
  if (!vec) { fprintf (stderr, "embed_text failed\n"); return; }
  uint i, n = len(vec);
  fprintf (stderr, "[embed_text] %d dimensions\n", n);
  for (i = 0; i < n; ++i) printf ("%f\n", vec[i]);
  free_vec (vec);
}

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);

  if (!strcmp(a(1), "-gen")) {
    char *model = "gemini-2.5-flash-lite";
    int i = 2;
    if      (!strcmp(a(i), "-0")) { model = "gemini-2.5-flash-lite";  ++i; }
    else if (!strcmp(a(i), "-1")) { model = "gemini-3-flash-preview"; ++i; }
    else if (!strcmp(a(i), "-9")) { model = "gemini-3.1-pro-preview"; ++i; }
    char *prompt = arg(i);
    if (!prompt) return fprintf (stderr, "missing prompt\n"), 1;
    char *result = generate_text (prompt, model);
    if (!result) return fprintf (stderr, "generate_text failed\n"), 1;
    printf ("%s\n", result);
    free (result);
  } else if (!strcmp(a(1), "-embed")) {
    if (!arg(2)) return fprintf (stderr, "missing text\n"), 1;
    do_embed (argv[2]);
  } else if (!strcmp(a(1), "-embed-file")) {
    char *text = read_file (a(2));
    do_embed (text);
    free (text);
  } else if (!strncmp(a(3), "embed", 5)) {
    embed_coll (argv[4], argv[1], a(3));
  } else {
    return fprintf (stderr, "%s", usage);
  }
  return 0;
}

#endif
