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

// -------------------- main (test) --------------------

#ifdef MAIN

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

char *usage =
  "gemini -embed \"text\"                 ... print embedding vector\n"
  "gemini -embed-file file.txt           ... embed contents of file\n"
  "gemini -gen [-0|-1|-9] \"prompt\"       ... generate text\n"
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
  } else {
    return fprintf (stderr, "%s", usage);
  }
  return 0;
}

#endif
