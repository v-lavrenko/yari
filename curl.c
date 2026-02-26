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

#include <curl/curl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "curl.h"
#include "vector.h"

// -------------------- libcurl response buffer --------------------

typedef struct { char *data; size_t len; } cbuf_t;

static size_t write_cb (void *ptr, size_t size, size_t nmemb, void *userdata) {
  size_t n = size * nmemb;
  cbuf_t *b = (cbuf_t *)userdata;
  b->data = realloc (b->data, b->len + n + 1);
  memcpy (b->data + b->len, ptr, n);
  b->len += n;
  b->data[b->len] = '\0';
  return n;
}

// -------------------- curl --------------------

// GET (payload=NULL) or POST to url with given headers.
// Returns malloc'd response body, or NULL on error. Caller frees.
char *curl (char *url, char **headers, char *payload) {
  // one-time global init
  static int inited = 0;
  if (!inited) { curl_global_init (CURL_GLOBAL_DEFAULT); inited = 1; }

  CURL *c = curl_easy_init();
  if (!c) { fprintf (stderr, "[curl] curl_easy_init failed\n"); return NULL; }

  cbuf_t response = {NULL, 0};
  struct curl_slist *hdrs = NULL;
  char **h = headers;
  while (h && *h) hdrs = curl_slist_append (hdrs, *h++);

  curl_easy_setopt (c, CURLOPT_URL,           url);
  curl_easy_setopt (c, CURLOPT_HTTPHEADER,     hdrs);
  if (payload)
    curl_easy_setopt (c, CURLOPT_POSTFIELDS,   payload);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION,  write_cb);
  curl_easy_setopt (c, CURLOPT_WRITEDATA,      &response);
  curl_easy_setopt (c, CURLOPT_TIMEOUT,        30L);

  CURLcode res = curl_easy_perform (c);
  curl_slist_free_all (hdrs);
  curl_easy_cleanup (c);

  if (res != CURLE_OK) {
    fprintf (stderr, "[curl] %s\n", curl_easy_strerror(res));
    free (response.data);
    return NULL;
  }
  return response.data; // caller frees
}

// -------------------- multi_post --------------------

// POST len(payloads) requests to the same url with the same headers, in parallel.
// payloads: vector of (char*). Returns vector of (char*) responses. Caller frees.
char **multi_post (char *url, char **headers, char **payloads) {
  int n = len (payloads);
  CURLM *multi = curl_multi_init();
  CURL **easies = calloc (n, sizeof(CURL*));
  cbuf_t *bufs  = calloc (n, sizeof(cbuf_t));

  struct curl_slist *hdrs = NULL;
  char **h = headers;
  while (h && *h) hdrs = curl_slist_append (hdrs, *h++);

  int i;
  for (i = 0; i < n; i++) {
    easies[i] = curl_easy_init();
    curl_easy_setopt (easies[i], CURLOPT_URL,           url);
    curl_easy_setopt (easies[i], CURLOPT_HTTPHEADER,     hdrs);
    curl_easy_setopt (easies[i], CURLOPT_POSTFIELDS,     payloads[i]);
    curl_easy_setopt (easies[i], CURLOPT_WRITEFUNCTION,  write_cb);
    curl_easy_setopt (easies[i], CURLOPT_WRITEDATA,      &bufs[i]);
    curl_easy_setopt (easies[i], CURLOPT_TIMEOUT,        30L);
    curl_multi_add_handle (multi, easies[i]);
  }

  int still_running = 0;
  do {
    curl_multi_perform (multi, &still_running);
    if (still_running) curl_multi_poll (multi, NULL, 0, 1000, NULL);
  } while (still_running);

  // check results: NULL out responses for failed transfers
  CURLMsg *msg;
  int msgs_left;
  while ((msg = curl_multi_info_read (multi, &msgs_left))) {
    if (msg->msg == CURLMSG_DONE && msg->data.result != CURLE_OK) {
      CURL *e = msg->easy_handle;
      for (i = 0; i < n; i++) {
        if (easies[i] == e) {
          fprintf (stderr, "[multi_post] request %d: %s\n",
                   i, curl_easy_strerror (msg->data.result));
          free (bufs[i].data);
          bufs[i].data = NULL;
          break;
        }
      }
    }
  }

  char **results = new_vec (n, sizeof(char*));
  for (i = 0; i < n; i++) {
    curl_multi_remove_handle (multi, easies[i]);
    curl_easy_cleanup (easies[i]);
    results[i] = bufs[i].data;
  }

  curl_slist_free_all (hdrs);
  curl_multi_cleanup (multi);
  free (easies);
  free (bufs);
  return results; // caller frees each results[i] and free_vec(results)
}
