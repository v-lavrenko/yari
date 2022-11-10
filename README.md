# mtx: a swiss-army knife for information retrieval

**mtx** is a command-line tool for rapidly trying new ideas in Information Retrieval and Machine Learning.

**mtx** is the right tool if you secretly wish you could: 

*  play with Wikipedia-sized datasets on your laptop 

*  do it interactively, like the boys whose data fits in Matlab

*  quickly test that too-good-to-be-true algorithm you see at SIGIR

*  try ungodly concoctions, like BM25-weighted PageRank over ratings

*  cache all intermediate results, so you never have to re-run a month-long job

*  use awk/perl to hack internal data structures half-way through a computation

**mtx** is made for *Unix hackers*. It is a shell tool, not a library or an application. It's designed for interactive use and relies on your shell's tab-completion and history features. For scripting it, I highly recommend [this](#scripting_with_makefiles).

The design is based on two ideas:
 1.  Almost anything in IR/ML can be represented as a sparse matrix: documents, queries, relevance judgments, inverted indices, ranked lists. It also works well for most types of image/video/speech features, for user-item ratings in recommender systems and for weighted edges in all kinds of networks. 
 2.  Most IR/ML algorithms can be re-written a sequence of matrix operations, interspersed with clever little hacks. The result is that almost any IR/ML experiment can be performed with a handful of **mtx** commands. See the table of contents on the right.

**mtx** is disk-hungry but memory-frugal. All matrices are stored in files and memory-mapped in small chunks. An RxC matrix will require at least 16R+8C bytes of available memory. Number or rows/columns cannot exceed 2`<sup>`32`</sup>`-1. **mtx** tries to be efficient in the most expensive operations (matrix multiplication and transpose). It is optimised for large, sparse matrices, but will handle dense ones as well. **mtx** is meant to be parallelised at the process level: 10 jobs using the same matrix will not require 10x the memory. 

# Importing data

Let's say we have some documents in a compressed file **docs.gz**:

	:::xml
	`<DOC id="cnn1">` [Manilla] The Philippines said on Monday that it had ... `</DOC>`
	`<DOC id="cnn14">` This one I think is called a Yink, he likes to wink ... `</DOC>`
	`<DOC id="bbc2">` Then am I a happy fly, if I live, or if I die ... `</DOC>`

We also have some queries in **qrys.txt**: 

	:::python
	# queryID followed by query text, all on the same line
	Q1 Dr. Zeuss
	Q3 William Blake

And TREC-style relevance judgments (**rels.rcv**):

	:::python
	#queryID documentID relevant? rest of the line is ignored
	      Q1      cnn14      1          # document "cnn14" is relevant to query Q1
	      Q1       bbc2      0          # non-relevant document (can be omitted)
	      Q3       bbc2      1          # "bbc2" is relevant to Q2


We'll represent this data as three matrices (**DOCS**, **QRYS** and **RELS**):

	:::python
	zcat docs.gz | mtx load:xml DOCS [DOCIDS x WORDS]    # each row is a document vector (bag of WORDS)
	cat qrys.txt | mtx load:txt QRYS [QRYIDS x WORDS]    # each row is a query vector (bag of WORDS)
	cat rels.rcv | mtx load:rcv RELS [QRYIDS x DOCIDS]   # each row is a list of relevant docs per query


We also created three hash-maps (DOCIDS, WORDS, QRYIDS), which map strings (words, ids) to numbers, e.g. "//cnn14//" -> 2, "//Monday//" -> 6, "//Q3//" -> 2((Note that each hash-map is independent, so different strings can map to the same number in two different maps. Likewise, the same string can map to two different numbers. It is your responsibility to ensure that matrix dimensions are compatible. The best way to do that is by using the appropriate hash-map when loading data into matrices.)) Matrices and hash-maps are directories with binary files. To see their contents, use the **mtx print** command:

	:::python
	ls DOCS DOCIDS WORDS                # each matrix is a set of binary files
	mtx print:rcv DOCS [DOCIDS x WORDS] # prints docid-word-tf triples (every non-zero cell in the matrix)
	mtx print:rcv DOCS                  # same, but uses internal numbers instead of words / document ids
	mtx print:rcv,top=10 DOCS           # prints 10 words with highest frequency for each document
	mtx print:xml DOCS [DOCIDS x WORDS] # might look like docs.xml, but without the original word order
	mtx print:svm DOCS                  # use as input to machine-learning packages (e.g. svmlight)


:!: For more detail see the [ Data Formats](yari/formats) page.
# Information Retrieval

Let's do a simple retrieval experiment. For each query, let's rank the documents according to how many times they contain query words. If **q** and **d** are vectors over words, the ranking score is simply a dot product **q . d**. We can compute these dot products for all queries and all documents by doing a cross-product between the query matrix (where **q** is a row) and the transpose of the document matrix (where **d** would be a column):

	:::python
	mtx transpose DOCS                                 # this creates DOCS.T, (an inverted index of DOCS)
	mtx SCORE = QRYS x DOCS.T                          # SCORE[q,d] = SUM_w QRYS[q,w] * DOCS[d,w] 
	mtx print:rcv SCORE [QRYIDS x DOCIDS]              # print all scores as triples: queryID documentID score


## Stemming and n-grams

The above was pretty basic, we didn't use stemming, and the ranking formula was SUM-tf. Now let's try something more realistic:

	:::python
	rm -rf DOCIDS WORDS QRYIDS                                            # always erase hash-maps when re-building
	zcat docs.gz | mtx load:xml DOCS [DOCIDS x WORDS] 'stop,stem=Krovetz' # remove stopwords, use Krovetz stemmer
	cat qrys.txt | mtx load:txt QRYS [QRYIDS x WORDS] 'stop,stem=Krovetz' # use same options for queries and docs


**mtx** comes with Porter, Krovetz, Spanish and Arabic stemmers built-in. But sometimes we need to work with a language that doesn't have a good stemmer. One good strategy is to replace all words with [ character 4-grams](http://dl.acm.org/citation.cfm?doid=1390334.1390518 ):

	:::python
	mtx print:rcv DOCS [DOCIDS x WORDS] \                                         # dump DOCID WORD weight triples
	| gawk '{ for (i=1; i<=length($2); ++i) print $1, substr($2,i,4), $3}' \      # split WORD into 4-byte substrings 
	| mtx load:rcv NDOCS [DOCIDS x NGRAMS]                                        # read result into a new matrix


It's easy to insert awk/perl scripts anywhere. Just make sure you keep track of matrix dimensions. For example, **NDOCS** has different dimensionality from **DOCS** and is no longer compatible with **QRYS**. You should always pre-process queries and documents in the same way. 
## BM25 and Language Models

Let's say we want use the BM25 ranking formula. All we need to do is apply an appropriate weighting to each document vector **d**, and then compute the dot-products **q . d** as before:

	:::python
	mtx BM25 = weigh:bm25,k=2,b=0.75 DOCS              # apply BM25 tf.idf weighting to every document (row) in DOCS
	mtx transpose BM25                                 # create BM25-weighted inverted lists
	mtx RANK_BM25 = QRYS x BM25.T                      # now inner product is the same as the BM25 score



And here's how to do query-likelihood language model:

	:::python
	mtx LMS = weigh:lm:k=100 DOCS                      # apply language-model weighting with Dirichlet prior of 100
	mtx transpose LMS                                  # create language-model-weighted inverted lists
	mtx RANK_LMS = QRYS x LMS.T                        # now inner product is the same as the query-likelihood model


## Recall / Precision plot

We can compare which one works better:

	:::python
	mtx print:evl RANK_BM25 RELS > BM25.evl            # evaluate BM25 rankings, dump for UMass trec_eval
	mtx print:evl RANK_LMS  RELS > LMS.evl             # evaluate LM rankings
	trec_eval -signtest BM25.evl LMS.evl               # compare LM against BM25, do a sifnificance test


:!: For more detail, see the [ Evaluation](yari/evaluation) page.

## Relevance Feedback

Let's use **RELS** in a Rocchio-like relevance-feedback algorithm (positive examples only):

	:::python
	mtx pRELS = weigh:uniform RELS                     # pRELS[q,d] = 1/N if document d is relevant to query q, otherwise 0
	mtx MEANS = pRELS x BM25                           # MEANS[q,w] = SUM_d pRELS[q,d] * BM25[d,w] 
	mtx RANK_RF = MEANS x BM25.T cosine                # use means as the new queries, use cosine similarity


We can also do a weighted combination, giving 10 times more weight to the original queries:

	:::python
	mtx MEANS = MEANS / 10                             # give a weight of 0.1 to the means
	mtx MEANS = MEANS + QRYS                           # add the original query vectors


## Query expansion 

Now let's do query expansion using top-ranked documents from each ranked list (Rocchio-style pseudo-feedback):

	:::python
	mtx TOP10 = QRYS x BM25.T top=10                   # TOP10 contains top 10 documents from each ranked list
	mtx MEANS = TOP10 x BM25 top=100                   # mean of top 10 documents for each query (top 100 terms)
	mtx RANK_QE = MEANS x BM25.T cosine                # use those means for a new round of retrieval


## Relevance Models 

	:::python
	mtx DLMS = weigh:L1 DOCS                          
	mtx LMS1 = weigh:lm:k=500 DOCS                     # language-model weighting with Dirichlet prior of 500
	mtx LMS2 = weigh:lm:b=0.5 DOCS                     # language-model with Jelinek-Mercer smoothing of 0.5
	mtx transpose LMS1                                 # create language-model-weighted inverted lists
	mtx transpose LMS2                                 # create language-model-weighted inverted lists
	mtx RET = QRYS x LMS1.T lm:d=500,docs=DOCS         # now inner product is the same as query-likelihood
	mtx TOP = log2posterior,top=10 RET                 # posterior over top 10 documents
	mtx RM0 = weigh:L1 QRYS                            # query model
	mtx RM1 = TOP x DLMS top=100                       # relevance model 1, truncated to top 100 terms
	mtx RM2 = 0.1 RM0 + 0.9 RM1                        # relevance model 3
	mtx RET = RM2 x LMS2.T                             # cross-entropy ranking


# Pairwise similarities

## Doc-to-doc affinity

Let's compute pairwise cosine similarities among all documents in **DOCS**:

	:::python
	mtx DOC_SIMS = DOCS x DOCS.T cosine,top=1000       # without 'top=1000' you may get a very large matrix
	mtx print:rcv DOC_SIMS [DOCIDS x DOCIDS]           # will dump the matrix in an easy-to-consume form


## Distributional semantics

NLP folks believe that terms that co-occur in many documents share the same meaning:

	:::python
	mtx WORD_SIMS = DOCS.T x DOCS cosine,top=1000      # the result is a word-by-word matrix
	mtx print:rcv WORD_SIMS [WORDS x WORDS]            # use the right hashmaps when printing


## Lp distance metrics

{{ :yari:minkowski.png?350|}}
Sometimes you want a more esoteric distance metric, which cannot be easily expressed as a dot-product. The following lines compute various pairwise negative distances between all documents. The figures to the right show the difference between a Euclidian (L2) distance and a Minkowski p-norm.

	:::python
	mtx L2 = DOCS dist DOCS            # negative Euclidian (L2) distance
	mtx L1 = DOCS dist:p=1 DOCS        # negative Manhattan (L1) distance
	mtx L0 = DOCS dist:p=0 DOCS        # negative Hamming (L0) distance
	mtx Lp = DOCS dist:p=0.75 DOCS     # negative Minkowski (p=0.75 norm)

Note that the distance operator takes **DOCS** on both sides (without the transpose).
## Gaussian, Laplace kernels

You can always combine these distances with different weightings. For example, the following computes Laplace density over InQuery-weighted document vectors, cropped to 100 most highly-weighted terms, with term statistics centered to zero mean and unit standard deviation:

	:::python
	mtx DOCS = weigh:inquery,top=100 DOCS              # apply InQuery weights, keep top 100 terms per document 
	mtx DOCS = weigh:std DOCS                          # center the data: each all weights have mean=0, st.dev=1
	mtx L1 = DOCS dist:p=1,top=1000 DOCS               # negative L1 distance, keep only 1000 nearest neighbours
	mtx print:rcv L1 \                                 # print triples: docno1 docno2 -L1 
	| gawk '{print $1, $2, exp($3/b)/(2*b)}' \         # convert -L1 to Laplace density: exp (-|x-m|/b) / 2b
	| mtx load:rcv LAPLACE_DISTANCE                    # load into a matrix, dimensions stay the same

Note: we're using a gawk script to apply a function that would be cumbersome to code with **mtx**. Why? Because it's easy, and it doesn't make the code any slower: the cost of computing the distance matrix dwarfs the cost of re-weighting.
# Clustering

## K-means

Let's cluster the documents in **DOCS** using the K-means algorithm:

	:::python
	mtx CENTROIDS = rnd:seed=0 50 WORDS                # centroids = 50 random vectors over WORDS
	
	for iteration in {1..5}
	do
	    mtx transpose CENTROIDS
	    mtx SIMS = DOCS x CENTROIDS.T cosine           # similarity of each document to each centroid
	    mtx SIMS = weigh:top=1,uniform SIMS            # pick most similar centroid, set its weight to 1
	    mtx transpose SIMS
	    mtx SIMS.T = weigh:uniform SIMS.T
	    mtx CENTROIDS = SIMS.T x DOCS                  # new centroid = average of documents assigned to it
	done
	
	mtx print:rcv SIMS.T '' x DOCIDS                   # print a list of documents in each cluster
	mtx print:rcv CENTROIDS '' x WORDS                 # print centroid of each cluster (words with weights)


A simpler formulation:

	:::python
	mtx CENTROIDS = rnd:seed=0 50 WORDS               # centroids = 50 random vectors over WORDS
	
	for iteration in {1..5}
	do
	    mtx SIMS = CENTROIDS x DOCS.T cosine          # similarity of each cluster (centroid) to each doc
	    mtx SIMS = colmax SIMS                        # assign doc to most similar cluster 
	    mtx SIMS = uniform SIMS                       # 
	    mtx CENTROIDS = SIMS x DOCS                   # centroid = average of documents assigned to it
	done
	
	mtx print:rcv SIMS '' x DOCIDS                    # print a list of documents in each cluster
	mtx print:rcv CENTROIDS '' x WORDS                # print centroid of each cluster (words with weights)



## Gaussian mixture (EM)

Make the following changes to the K-means recipe above:

	:::python
	    mtx SIMS = DOCS x CENTROIDS.T euclidian        # Euclidian distance instead of cosine
	    mtx SIMS = weigh:rbf=1,L1 SIMS                 # radial-basis function with a variance of 1


Note: this assumes identity as the covariance matrix. Pick a good value for variance.


# Classification

Assume we have the following three matrices:


*  **TRAIN** [docs x features] ... each row is a feature vector for a training instance

*  **TEST** [docs x features] ... each row is a feature vector for a testing instance

*  **LABELS** [docs x classes] ... rows are training documents, columns are class labels, values should be 0/1

## K Nearest Neighbours

	:::python
	mtx transpose TRAIN
	mtx SIMS = TEST x TRAIN.T euclidian                # testing x training similarity matrix
	mtx SIMS = weigh:top=K,uniform SIMS                # take K nearest matches, assign uniform weights
	mtx PREDICTED = SIMS x LABELS                      # count labels from nearest matches
	mtx print:rcv,top=1 PREDICTED                      # print the most likely class (top 1) for each doc


## Radial Basis Kernels

Make the following change to the above recipe and get a kernelized RBF classifier:

	:::python
	mtx SIMS = weigh:rbf=1,L1 SIMS 


## Gaussian Centroid

The following doesn't use priors, but they're trivial to add.

	:::python
	mtx transpose LABELS                               # list of positive documents for each class
	mtx LABELS.T = weigh:uniform LABELS.T              # so weights of documents add up to 1
	mtx CENTROIDS = LABELS.T x TRAIN                   # mean of positive documents for each class
	mtx transpose CENTROIDS
	mtx PREDICTED = TEST x CENTROIDS.T euclidian       # distance from each testing doc to each class
	mtx PREDICTED = weigh:rbf=1 PREDICTED              # convert distance to Gaussian density
	mtx print:rcv,top=1 PREDICTED                      # print the most likely class (top 1) for each doc


## ROC curve / F1 measure

Let's see how well our classifiers are doing. We'll need the matrix **TEST_LABELS**, in the same format as **LABELS**:

	:::python
	mtx transpose PREDICTED                            # a list of docs (with confidence scores) for each class
	mtx print:roc PREDICTED.T TEST_LABELS 'macro'      # produces macro-averaged ROC curve, recall, precision, F1


## Logistic Regression

This example implements regularised logistic regression with L2 regularisation. The derivative of the likelihood is zero when ∑(y-σ(**w·x**))x = 2λ**w**, which we can use as an update rule. Assume the matrix **TRAIN** [DOCIDS x WORDS] contains the training document vectors **x** (which must include a constant "bias" term). Assume **LABELS** [1 x DOCIDS] contains y = 1 or 0, indicating positive/negative documents (note: LABELS are different from above sections). Assume regularisation parameter λ=0.5 for convenience.

	:::python
	'ERRORS BELOW'
	mtx transpose TRAIN
	mtx WEIGHTS = rnd:sphere 1 WORDS           # start with a random [1 x WORDS] vector of weights w
	for iteration in {1..5}
	do
	  mtx PREDICTED = WEIGHTS x TRAIN.T        #                       w·x       w·x for every training x 
	  mtx PREDICTED = sig=1 PREDICTED          #                     σ(w·x)      predicted class for x
	  mtx LOGLOSS = LABELS - PREDICTED         #                 y - σ(w·x)      logistic loss
	  mtx WEIGHTS = LOGLOSS x TRAIN            #              ∑ (y - σ(w·x)) x   batch update 
	done


For multi-class, everything is the same except **LABELS** [CLASSES x DOCIDS] contains one row of y = 1 or 0 for each class:

	:::python
	'ERRORS BELOW'
	mtx transpose TRAIN
	mtx WEIGHTS = LABELS x TRAIN               # initialise each hyperplane with a class centroid (may want to L1 labels)
	for iteration in {1..5}
	do
	  mtx PREDICT = WEIGHTS x TRAIN.T          #                       w·x       w·x for every training x 
	  mtx transpose PREDICT                    #                                 softmax operates on rows
	  mtx PREDICT = softmax PREDICT.T          #               softmax(w·x)      σ(w·x) = exp(w·x) / ∑ exp(w·x) 
	  mtx transpose PREDICT                    #                                 transpose back
	  mtx LOGLOSS = LABELS . PREDICT.T         #             y softmax(w·x)
	  mtx LOGLOSS = LABELS - LOGLOSS           #        y [1 - softmax(w·x)]
	  mtx WEIGHTS = LOGLOSS x TRAIN            #      ∑ y [1 - softmax(w·x)] x
	done

## Max-Margin Hyperplane

The following example implements a *batch* version of the [Passive Aggressive](http://jmlr.csail.mit.edu/papers/volume7/crammer06a/crammer06a.pdf) algorithm for the non-separable case. Let **TRAIN** [DOCIDS x WORDS] contain the training document vectors **x** and let **LABELS** [1 x DOCIDS] contain y = +1 or -1, indicating positive/negative documents (note: **LABELS** are different from above sections). Assume margin size ε=0.1 and misclassification cost C=0.5

	:::python
	mtx NORM = sum2 TRAIN                       # |x|² for every training document x
	mtx NORM = NORM + 0.5                       # |yx|² + C, since y = +1 or -1
	mtx transpose TRAIN
	mtx HYPERPLANE = LABELS x TRAIN             # Rocchio initialisation: sum.positives - sum.negatives
	for i in {1..5}
	do
	  mtx PREDICTED = HYPERPLANE x TRAIN.T      #          (w·x)                  prediction for each example
	  mtx AGREEMENT = PREDICTIONS . LABELS      #        y (w·x)                  >0 if correct, <0 if wrong
	  mtx LOSS      = 0.1 - AGREEMENT           #    ε - y (w·x)                  ε-insensitive hinge loss
	  mtx POS_LOSS  = LOSS '>' 0                #                                 
	  mtx LOSS      = LOSS . POS_LOSS           #   [ε - y (w·x)]                 remove negative loss values
	  mtx DIRECTION = LOSS . LABELS             #   [ε - y (w·x)] y
	  mtx DIRECTION = DIRECTION / NORM          #   [ε - y (w·x)] y  / (C+|yx|²) 
	  mtx GRADIENT  = DIRECTION x TRAIN         # Σ [ε - y (w·x)] yx / (C+|yx|²)  batch version, not online
	  mtx HYPERPLANE = HYPERPLANE + GRADIENT    #                                 update the hyperplane
	done


Now make **LABELS** be [DOCIDS x CLASSES], and you will get a **multi-class** classifier without writing any extra code.
# Recommender systems

Assume that we have an item-by-user matrix **ratings**, where each row is a set of ratings for a particular item. We'll do a combination of two popular approaches: **item-to-item** and **user-to-user**:

## Item-to-item approach

Assume user's rating for the item depends on how this user rated other items similar to this one:

	:::python
	mtx item_sims = ratings x ratings.T cosine,top=20  # for each item: 20 items with most similar rating profiles
	mtx item_to_item = item_sims x ratings             # for each item: average user preferences over similar items


## User-to-user approach

Assume user's rating for an item depends on how this item was rated by other users with similar tastes:

	:::python
	mtx transpose ratings
	mtx user_sims = ratings.T x ratings cosine,top=50  # for each user: 50 users with most similar preferences
	mtx user_to_user = user_sims x ratings.T           # for each user: average item preferences from similar users


## Combining predictions

Now let's combine the two approaches:

	:::python
	mtx item_to_item = 0.7 . item_to_item              # we'll trust this user's other ratings more (weight of 0.7)
	mtx user_to_user = 0.3 . user_to_user              # and give less weight (0.3) to ratings from other users
	mtx transpose user_to_user
	mtx predicted = item_to_item + user_to_user.T      # average of item-to-item and user-to-user estimates
	mtx print:rcv predicted                            # show predictions


## Mean-squared error

To see how well we're doing, we need an item-by-user matrix **test_ratings**, in the same format as **ratings**. 

	:::python
	mtx test_predicted = predicted '&' test_ratings    # keep only the user-item pairs for which we know true rating
	mtx test_errors = test_predicted - test_ratings     # difference between predicted and true rating for each pair
	mtx print:rcv test_errors | gawk '{num_pairs++; sum_errors += $3*$3} END {print sum_errors / num_pairs}'



# Network analysis

Suppose we have a small graph with 10 nodes **a,b,c,d,e,v,w,x,y,z**, defined by the following edge matrix:

{{ :yari:path0.png?500|}}

	:::python
	# a  b  c  d  e  v  w  x  y  z
	  0  1  1  0  1  0  0  0  0  0 
	  1  0  1  1  0  0  0  0  0  0 
	  1  1  0  1  1  0  0  0  0  0 
	  0  1  1  0  1  1  0  0  0  1 
	  1  0  1  1  0  0  0  0  0  0 
	  0  0  0  1  0  0  1  1  0  1  
	  0  0  0  0  0  1  0  1  1  0 
	  0  0  0  0  0  1  1  0  1  1 
	  0  0  0  0  0  0  1  1  0  1 
	  0  0  0  1  0  1  0  1  1  0 


If the above matrix is stored in a plain text file **graph.csv**, we can load it into a matrix as follows:

	:::python
	cat graph.csv | mtx load:csv GRAPH


For larger graphs, it's easier to represent them in sparse form, for example **graph.xml**, where each document represents a node and words in that document represent out-going edges:

	:::xml
	`<DOC id="a">` b c e `</DOC>` 
	`<DOC id="b">` a c d `</DOC>`
	...


Loading it into a matrix:

	:::python
	cat graph.xml | mtx load:xml GRAPH [NODES x NODES]

## Shortest path / reachability

If we multiply the matrix **GRAPH** by itself, then cell **[i,j]** will contain the number of paths of length 2 between node **i** and node **j**. If the number is zero, this means **i** is not reachable from **j** in 2 steps. We can keep multiplying to count paths of length 3, 4, 5 ...

	:::python
	mtx PATHS = GRAPH x GRAPH                # now PATHS[i,j] = number of 2-step paths between i and j
	mtx PATHS = PATHS x GRAPH                # now PATHS[i,j] = number of 3-step paths between i and j
	mtx print:csv PATHS | sed 's/\.00//g'    # print result, sed truncates floating point to integers

	:::python
	# a  b  c  d  e  v  w  x  y  z           
	  4  8  8  4  8  3  0  0  0  3           # nodes w,x,y are not reachable from a in 3 steps
	  8  4  8 10  4  2  1  2  1  2           # every node is reachable from b in 3 steps
	  8  8  8 10  8  3  1  2  1  3           # there are two ways to get from c to x: either via v or via z
	  4 10 10  6 10  9  4  4  4  9
	  8  4  8 10  4  2  1  2  1  2
	  3  2  3  9  2  6  9 10  5 10
	  0  1  1  4  1  9  4  8  8  5
	  0  2  2  4  2 10  8  8  8 10
	  0  1  1  4  1  5  8  8  4  9
	  3  2  3  9  2 10  5 10  9  6


## Graph cutting

{{ :yari:graph-cut.png|}}
Graph cutting is needed if you want to separate the motorcyclist from the car in the frame on the right. The two are connected because they are moving with the same speed in the same direction and have patches with similar appearance. However, there are fewer *inter-connections* between the car and the motorcyclist than there are *intra-connections*.

We can separate the two by identifying strongly-connected sub-graphs via the following procedure:

	:::python
	mtx PATHS = GRAPH x GRAPH                    # all paths of length exactly two
	mtx PATHS = PATHS . GRAPH                    # project paths to existing edges
	mtx transpose PATHS
	mtx PATHS = PATHS + PATHS.T                  # symmetrize 
	mtx PATHS = PATHS + GRAPH                    # add paths of length 1
	mtx print:csv PATHS | sed 's/\.00//g'

{{ :yari:path1a.png?500|}}

	:::python
	# a  b  c  d  e  v  w  x  y  z
	  0  3  5  0  3  0  0  0  0  0
	  3  0  5  3  0  0  0  0  0  0
	  5  5  0  5  5  0  0  0  0  0
	  0  3  5  0  3  3  0  0  0  3
	  3  0  5  3  0  0  0  0  0  0
	  0  0  0  3  0  0  3  5  0  5
	  0  0  0  0  0  3  0  5  3  0
	  0  0  0  0  0  5  5  0  5  5
	  0  0  0  0  0  0  3  5  0  3
	  0  0  0  3  0  5  0  5  3  0


By throwing away edges with smallest weights, we can cut the graph into two strongly-connected components: **{a,b,c,d,e}** and **{v,w,x,y,z}**. For larger graphs, you will need to iterate a few times. 
## PageRank

{{ :yari:pagerank-eq.png?150|}}
Suppose that **GRAPH [NODES x NODES]** contains a directed graph (the one above was undirected), and let's assume that a row contains *outgoing* links for each node. In the PageRank algorithm each node **y** shares it's PR equally across outgoing links, and each node **x** accumulates the contributions from incoming links. Here's one way to compute PageRank (lambda=0.9):

	:::python
	mtx PR = ones 1 NODES                             # PageRank (PR) will be a 1 x NODES row vector
	mtx PR = uniform PR                               # start with a uniform PR for each node
	mtx BG = PR . 0.1                                 # smoothing component: (1-lambda) / NODES
	mtx  G = uniform GRAPH                            # G [y,x] = 1/#out(y) if y points to x, otherwise 0
	for iteration in {1..5}
	do
	  mtx PR = PR x G                                 # PR[x] = SUM_y PR[y] * G[y,x]
	  mtx PR = PR . 0.9               
	  mtx PR = PR + BG                                # add the smoothing component
	done
	mtx print:rcv PR '' x NODES                       # print the results

## Hubs and Authorities

Assume **GRAPH** is as above. A good *hub* node points to many *authoritative* pages. A good *authority* is referenced by many good *hubs*. Below is the HITS algorithm:

	:::python
	mtx HUBS = ones 1 NODES                           # hub/authority scores: 1 x NODES row vectors
	mtx AUTH = ones 1 NODES
	mtx transpose GRAPH                               # GRAPH.T[x,y] = 1 if x is pointed to by y
	for iteration in {1..5}
	do
	  mtx AUTH = HUBS x GRAPH                         # AUTH[x] = SUM_y HUBS[y] * 1_if_y_points_to_x 
	  mtx AUTH = weigh:L2 AUTH                        # normalize
	  mtx HUBS = AUTH x GRAPH.T                       # HUBS[y] = SUM_x AUTH[x] * 1_if_x_pointed_to_by_y
	  mtx HUBS = weigh:L2 HUBS                        # normalize
	done
	mtx print AUTH '' x NODES                         # print results
	mtx print HUBS '' x NODES


# Probabilistic Models

## Topic Models (pLSI)

Probabilistic Latent Semantic Indexing is a latent-factor model for discovering topics in text. Each topic **z** defines a topic model P(**w**|**z**) over the words **w**, and each document **d** is characterised a unique mixture of topics P(**z**|**d**). pLSI assigns the following log-likelihood to a collection of documents: ∏`<sub>`d`</sub>` ∏`<sub>`w**∈**d`</sub>` ∑`<sub>`z`</sub>` P(**w**|**z**) P(**z**|**d**)

After some algebraic manipulation, the pLSI estimation algorithm can be rewritten as follows:

*  P(**z**|**d**) ~ P(**z**|**d**) ∑`<sub>`w`</sub>` P(**w**|**z**) N(**w**,**d**) / P(**w**|**d**)

*  P(**w**|**z**) ~ P(**w**|**z**) ∑`<sub>`d`</sub>` P(**z**|**d**) N(**w**,**d**) / P(**w**|**d**)

*  P(**w**|**d**) = ∑`<sub>`z`</sub>` P(**w**|**z**) P(**z**|**d**)
Here N(**w**,**d**) is the number of times we see word **w** in document **d**, and the **~** operator implies equality after the right-hand-side is normalised to unit length over the appropriate variable.

	:::python
	mtx Ndw = DOCS + 0                #   [docs x words] word frequency matrix, Ndw[d,w] = N(w,d), initialised from DOCS
	mtx Pdw = weigh:L1 Ndw            #   [docs x words] document model matrix, Pdw[d,w] = P(w|d), initialized from N(w,d)
	mtx Pzw = rnd:simplex WORDS 100   # [topics x words] topic language models, Pzw[z,w] = P(w|z), initialised randomly
	mtx Pdz = rnd:simplex 100 DOCIDS  #  [docs x topics] topic mixture matrix,  Pdz[d,z] = P(z|d), initialised randomly 
	for iteration in {1..5}
	do
	  mtx transpose Pdz
	  mtx transpose Pzw
	  mtx _dw = Ndw / Pdw             # [docs x words], helper matrix   _dw[d,w] =                       N(w,d) / P(w|d)
	  
	  mtx _dz = _dw x Pzw.T           # [docs x topics], helper matrix  _dz[d,z] =          ∑_w P(w|z) * N(w,d) / P(w|d)
	  mtx Pdz = _dz . Pdz             # [docs x topics], topic mixture  Pdz[d,z] = P(z|d) = ∑_w P(w|z) * N(w,d) / P(w|d)
	  mtx Pdz = weigh:L1 Pdz          # make ∑_z P(z|d) = 1
	  
	  mtx _zw = Pdz.T x _dw           # [topics x words], helper matrix _zw[z,w] =          ∑_d P(z|d) * N(w,d) / P(w|d)
	  mtx Pzw = Pzw   . _zw           # [topics x words], topic models  Pzw[z,w] = P(w|z) = ∑_d P(z|d) * N(w,d) / P(w|d)
	  mtx Pzw = weigh:L1 Pzw          # make ∑_w P(w|z) = 1
	
	  mtx Pdw = Pdz x Pzw             # [docs x words], document models Pdw[d,w] = P(w|d) = ∑_z P(w|z) P(z|d) 
	done
	mtx print:rcv Pdz DOCIDS x '' top=5       # show 5 most prominent topics for each document
	mtx print:rcv Pzw '' x WORDS top=20       # show 20 most-representative words for each topic

## Auto Tagging (CMRM)

Download and pre-process the dataset:

	:::python
	wget -c 'http://kobus.ca/research/data/eccv_2002/ECCV_2002_data.tar.gz'
	tar xzvf ECCV_2002_data.tar.gz
	cd eccv_2002
	paste AAA BBB | gawk 'BEGIN \
	{while (getline < "CCC") c[++b]=$1;} \
	{printf "%s", $1; for (i=2;i`<=NF;++i) printf " %s", c[$i]; print "";}' >` DDD
	#        AAA               BBB                   CCC                  DDD
	#        image_nums        document_blobs        cluster_membership > train.blobs.txt
	#        image_nums        document_words        words              > train.words.txt
	# test_1_image_nums test_1_document_blobs test_1_cluster_membership >  test.blobs.txt
	# test_1_image_nums test_1_document_words test_1_words              >  test.words.txt


	:::python
	mtx load:xml trainB [IMAGES x BLOBS] < train.blobs.xml
	mtx load:xml trainW [IMAGES x WORDS] < train.words.xml
	mtx load:xml  testB [IMAGES x BLOBS] <  test.blobs.xml
	mtx load:xml  testW [IMAGES x WORDS] <  test.words.xml
	mtx trainBw = weigh:lm:b=0.5 trainB # .01 .1 .2 .5 .8 .9 .99
	mtx transpose trainBw
	mtx SIM = testBw x trainBw.T
	mtx SIM = weigh:top=10 SIM # 5 10 20 50
	mtx SIM = log2posterior SIM
	mtx RESULT = SIM x trainW
	mtx print RESULT [IMAGES x WORDS]



## Image Annotation (CRM)

{{ :yari:crm-kernel.png?200|}}
{{ :yari:crm-joint.png?200|}}
The Continuous Relevance Model (CRM) is a non-linear doubly non-parametric model for image annotation. The model involves computing a joint probability distribution between a set of words **w** and a set of visual feature vectors **f** according to the equations on the right. The following example implements the CRM for the Corel dataset, where there are 4500 training / 500 testing images, each image is segmented into (up to) 10 regions, and each region is represented by 36 real-valued features.

	:::python
	mtx load:txt W < trainWords.txt                    # 4500 x 270 ...   trainImage w1 w2 ...
	mtx load:csv A < trainFeatures.csv                 # 45000 x 36 ...              f1 f2 ... f36
	mtx load:csv B <  testFeatures.csv                 #  5000 x 36 ...              f1 f2 ... f36
	mtx load:txt D < trainRegions.txt                  # 4500 x 45000 ... trainImage r1 r2 ...
	mtx load:txt F <  testRegions.txt                  #  500 x  5000 ...  testImage r1 r2 ...
	mtx D = weigh:L1 D  
	mtx C = B dist:p=0.75 A          #                                 -|fi - fj|^p
	mtx C = C / beta                 #                                 -|fi - fj|^p / β     = log P(fi|fj)
	mtx E = C dist:logSexp,full D    #                log 1/n ∑_j exp {-|fi - fj|^p / β}    = log P(fi|J)
	mtx G = F x E                    #            ∑_i log 1/n ∑_j exp {-|fi - fj|^p / β}    = log ∏ P(fi|J)     = log P(I|J) 
	mtx H = log2posterior G          #            1/z ∏_i 1/n ∑_j exp {-|fi - fj|^p / β}    = P(I|J) / ∑P(I|J)  = P(J|I)     
	mtx R = G x W                    # ∑_J P(w|J) 1/z ∏_i 1/n ∑_j exp {-|fi - fj|^p / β}    = ∑ P(w|J) P(J|I)   = P(w|I)     

# Random indexing / LSH

Suppose we want to detect near-duplicates in a very large collection **DOCS**. We can compute LSH fingerprints, which will be identical for very similar docs. Assume **DOCS** has dimensions **[DOCIDS x WORDS]**:

	:::python
	mtx HYPERPLANES = rnd:sphere 128 WORDS             # 128 WORDS-dimensional hyperplanes, sampled from a sphere
	mtx transpose HYPERPLANES
	mtx SIMS = DOCS x HYPERPLANES.T                    # for each doc: dot-product against these 256 hyperplanes
	mtx BINS = lsh:L=8 SIMS                            # quantize into 8 fingerprints (each 16 bits long)
	mtx print:rcv BINS DOCIDS x ''                     # any repeated fingerprint indicates a potential duplicate


If you don't care about proper sampling, you can use random indexing to achieve a similar result in one step (and a lot faster):

	:::python
	mtx BINS = simhash:L=8,k=16 DOCS                   # directly generate 8 x 16bit fingerprints for each doc



# Scripting with Makefiles

When you work with large datasets, simple operations take a long time. The last thing you want is to have a bug half-way through your algorithm -- it will take you hours/days/weeks to discover the bug, and then just as much time to run it again. One way to avoid this is to break your algorithm into stages and cache the output of every stage. An even better idea is to introduce dependencies between stages (e.g. stage 5 needs the output of stages 1 and 4), and let GNU make figure out which stages need to be re-done, and which don't. 

The example below shows how we can use a Makefile to construct a recommender system that combines collaborative filtering with a content-based approach to predict new ratings. Using a Makefile allows us to selectively execute just the right stages of the algorithm:


*  type **make**, and it will execute all stages in the right order

*  type **make -j 2**, and it will do content-based and collaborative approaches in two parallel threads

*  change **itemText.gz** and type **make** -- it'll re-do the content-based part, but not the collaborative one

*  change a parameter (0.7 -> 0.5), and type **touch collaborative; make** -- it'll re-do just the last stage 

	:::make
	all: predictions                                                    # overall goal is to generate predictions
	
	predictions: collaborative content_based                            # predictions depend on two stages:
	        mtx collaborative = 0.7 . collaborative                     # collaborative: items which are rated similarly
	        mtx content_based = 0.3 . content_based                     # content-based: items with similar text content
	        mtx predictions = collaborative + content_based
	
	collaborative: ratings                                              # collaborative stage needs item ratings
	        mtx rating_sims = ratings x ratings.T 'cosine,top=20'
	        mtx collaborative = rating_sims x ratings
	
	content_based: ratings content                                      # content-based stage needs ratings and content
	        mtx content_sims = content.T x content 'cosine,top=50'
	        mtx content_based = content_sims x ratings
	
	ratings: itemRatings.rcv                                            # item ratings are loaded from a file
	        cat $^ | mtx load:rcv ratings [items x items]
	        mtx transpose ratings
	        
	content: itemText.gz                                   
	        zcat $^ | mtx load:xml content [items x words]              # content is synchronised to ratings via 'items'
	        mtx content = 'weigh:bm25,k=2,b=0.75' content               # use appropriate weighting for text 
	        mtx transpose content