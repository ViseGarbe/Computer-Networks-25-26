# Glossary of Terms to Know from the PDFs

This glossary was curated from all 9 PDFs in the folder.
It keeps the technical vocabulary, acronyms, models, datasets, tools, metrics, and privacy/NLP concepts worth learning.
It intentionally leaves out administrative course-management terms and bibliography noise.

## Core IR Foundations

| Term | Meaning | Why it matters |
| --- | --- | --- |
| Information Retrieval (IR) | The task of finding documents that satisfy a user's information need. | It is the core subject of most of the PDFs. |
| Information need | The underlying need a user wants to satisfy, which may be broader than the typed query. | Many IR problems come from the gap between need and wording. |
| Query | The text entered by the user to express a need. | Nearly every model in the course starts from the query. |
| Corpus | A collection of documents to search or analyze. | IR systems are always built and evaluated over a corpus. |
| Relevance | The degree to which a document satisfies a user's need. | Retrieval effectiveness is fundamentally about relevance. |
| Ranking function | A scoring function that orders documents for a query. | Every retrieval model eventually needs one. |
| Vocabulary | The set of distinct terms used in a collection or model. | It defines the feature space of many IR and NLP methods. |
| Token | A unit produced by text processing, such as a word, subword, or symbol. | Token-level processing is the basis of indexing and NLP pipelines. |
| Type | A unique token form, as opposed to a token occurrence. | The token/type distinction appears in ATA and corpus analysis. |
| Tokenization | The process of splitting text into tokens. | Bad tokenization leads to bad indexing and bad NLP input. |
| N-gram | A contiguous sequence of n items, such as words or characters. | N-grams are a basic representation device in IR and NLP. |
| Stemming | Reducing words to a crude root by stripping prefixes or suffixes. | It increases matching, but can also reduce linguistic precision. |
| Lemmatization | Reducing words to their dictionary base form using linguistic information. | It is more precise than stemming and often more semantically faithful. |
| Part-of-Speech (POS) | The grammatical category of a word, such as noun, verb, or adjective. | POS information is useful for linguistic analysis and lemmatization. |
| Stop word | A very common word often filtered out during indexing. | Stop-word handling affects both efficiency and effectiveness. |
| Inverted index | A data structure that maps each term to the documents containing it. | It is the standard backbone of scalable keyword search. |
| Postings list | The list of documents associated with a term in an inverted index. | Many retrieval operations are implemented over postings. |
| DOCNO / Document ID | The unique identifier of a document in a collection. | You need it for runs, qrels, evaluation, and debugging. |
| Bag of Words (BoW) | A representation that records term occurrence while ignoring word order. | It is a classic baseline and a contrast point for modern neural methods. |
| Sparse representation | A vector where most dimensions are zero. | Classical term vectors and sparse neural models use it. |
| Dense representation | A compact real-valued vector with information spread across dimensions. | Dense retrieval and embeddings depend on it. |
| One-hot encoding | A sparse vector with a single active dimension. | It is a simple but extremely sparse way to represent symbols. |
| Term Frequency (TF) | How often a term appears in a document. | TF is a core signal in classical ranking. |
| Document Frequency (DF) | The number of documents containing a term. | DF helps estimate how discriminative a term is. |
| Inverse Document Frequency (IDF) | A weight that increases the importance of rare terms. | IDF is central in TF-IDF and BM25-style scoring. |
| TF-IDF | A weighting scheme that combines TF with IDF. | It is one of the most important classical term-weighting ideas. |
| Cosine similarity | A similarity measure based on the angle between two vectors. | It is the standard similarity in vector space models and embeddings. |

## Evaluation and Experimentation

| Term | Meaning | Why it matters |
| --- | --- | --- |
| Run | A system's ranked list of results for one or more queries. | Runs are what you submit and evaluate. |
| Relevance judgments (qrels) | Ground-truth labels saying whether a document is relevant to a topic. | Without qrels, you cannot compute standard IR metrics. |
| TREC | A major IR evaluation campaign and benchmark tradition. | Many datasets, formats, and evaluation practices come from TREC. |
| CLEF | A major evaluation initiative, especially strong in multilingual and European IR tasks. | It is directly relevant to the homework workflow described in the PDFs. |
| Pooling | Building a judgment set by merging top documents from multiple systems. | It is how relevance assessment is usually made scalable. |
| Binary relevance | A relevance scheme with only two states: relevant or not relevant. | Several classic metrics assume it. |
| Graded relevance | A multi-level relevance scheme, not just yes/no. | Metrics like DCG and nDCG rely on it. |
| Precision | The fraction of retrieved documents that are relevant. | It measures result quality among what was returned. |
| Recall | The fraction of relevant documents that were retrieved. | It measures coverage of the relevant material. |
| F-measure / F1 | The harmonic mean of precision and recall. | It summarizes a precision/recall trade-off in one value. |
| Precision@k (P@k) | Precision computed only over the top k results. | It matches how users often inspect only the first few results. |
| Recall@k (R@k) | Recall computed over the top k results. | It is useful when the ranking cutoff matters. |
| R-Precision | Precision measured at rank R, where R is the number of relevant documents. | It is a classic metric tying cutoff to the topic's recall base. |
| Average Precision (AP) | The average of precision values at the ranks where relevant documents appear. | It rewards systems that rank relevant documents early. |
| Mean Average Precision (MAP) | The mean AP over multiple queries. | It is one of the most important IR effectiveness measures. |
| Reciprocal Rank / Mean Reciprocal Rank (MRR) | The inverse of the rank of the first relevant result, averaged over queries. | It is especially important for answer-oriented tasks and MS MARCO. |
| Discounted Cumulative Gain (DCG) | A gain-based metric that discounts usefulness by rank position. | It handles graded relevance naturally. |
| Normalized DCG (nDCG) | DCG normalized by the ideal ranking. | It makes DCG comparable across queries. |
| Rank-Biased Precision (RBP) | A rank metric with a user persistence parameter controlling how deep users look. | It gives a realistic user model for ranked browsing. |
| trec_eval | A standard tool for computing IR evaluation metrics from runs and qrels. | It is the practical bridge between system output and metric values. |

## Tools and Collections

| Term | Meaning | Why it matters |
| --- | --- | --- |
| Apache Lucene | A Java library for indexing and searching text. | It is the main hands-on tool in the practical PDF. |
| PyLucene | A Python wrapper around Java Lucene. | It lets you use Lucene concepts from Python. |
| Analyzer | In Lucene, the component that tokenizes and filters text. | The analyzer defines how text is normalized for indexing and search. |
| Similarity (Lucene) | The Lucene scoring component implementing a ranking model. | It determines how Lucene scores documents. |
| TIPSTER | A classic large text collection of news and government material. | It is used as a realistic corpus in the hands-on material. |
| MS MARCO | A large benchmark for passage/document ranking. | It is one of the central datasets in modern neural IR. |
| ir_datasets | A library for loading standard IR datasets. | It simplifies reproducible experimentation. |
| ir_measures | A library for computing IR evaluation metrics. | It makes evaluation scripting cleaner and more consistent. |

## Traditional IR Models

| Term | Meaning | Why it matters |
| --- | --- | --- |
| Boolean model | A retrieval model based on exact Boolean logic over terms. | It is historically fundamental and still useful in some domains. |
| Vector Space Model (VSM) | A model representing queries and documents as weighted vectors. | It is the classical ranked alternative to Boolean retrieval. |
| Partial matching | Matching some, but not necessarily all, query terms. | It is one of the key advantages of ranked retrieval over strict Boolean logic. |
| Relevance feedback (RF) | Adjusting the query using information about relevant and non-relevant documents. | It is a classic way to improve retrieval after an initial ranking. |
| Rocchio algorithm | A vector-space relevance feedback method that moves the query toward relevant documents and away from non-relevant ones. | It is the canonical RF algorithm to know. |
| Pseudo-relevance feedback | Feedback that assumes the top-ranked documents are relevant even without human labels. | It is widely used because explicit feedback is rare. |
| Query drift | The failure mode where feedback moves the query away from the original intent. | It is the main risk of pseudo-relevance feedback. |
| Probabilistic model | A retrieval family that ranks documents by estimated relevance probability. | It is the conceptual basis of BM25 and language-model approaches. |
| Binary Independence Model (BIM) | A probabilistic model assuming binary term occurrence and term independence. | It is a classic precursor to BM25-style ranking. |
| BM25 | A strong probabilistic ranking function balancing TF, IDF, and document length. | It is the main classical baseline even in modern IR. |
| Language model (LM) | A model that treats a document as a probability distribution over words. | It is the basis of query-likelihood retrieval. |
| Query likelihood | Ranking documents by the probability that their language model could generate the query. | It is the standard retrieval formulation for language models. |
| Maximum Likelihood Estimation (MLE) | Estimating probabilities directly from observed counts. | It is the unsmoothed starting point for language-model scoring. |
| Smoothing | Adjusting probability estimates to avoid zeros and reduce sparsity problems. | Language-model retrieval depends on it. |
| Jelinek-Mercer smoothing | A smoothing method that interpolates the document model with the collection model. | It is one of the standard LM smoothing techniques. |
| Dirichlet smoothing | A smoothing method based on a Dirichlet prior over multinomial probabilities. | It is another standard and often very effective LM smoother. |
| Background model | The collection-wide language model used during smoothing. | It provides general collection statistics when document evidence is weak. |

## Privacy and Query Obfuscation

| Term | Meaning | Why it matters |
| --- | --- | --- |
| Privacy-utility trade-off | The idea that stronger privacy often reduces usefulness or accuracy. | It is the central tension in the privacy PDF. |
| Threat model | The assumptions you make about the attacker and the environment. | Privacy claims are meaningless without a threat model. |
| Honest-but-curious | An actor that follows the protocol correctly but still tries to learn as much as possible. | It is the threat model used for search-engine privacy in the slides. |
| Query obfuscation | Hiding sensitive query information by rewriting or perturbing the query. | It is the main privacy application discussed in the PDF. |
| Obfuscation mechanism | The procedure that transforms the original text/query into a protected one. | It is the operational unit of privacy-preserving text transformation. |
| Heuristics-based mechanism | An obfuscation strategy based on rules, synonyms, or local heuristics rather than formal guarantees. | The slides contrast these methods with DP-based ones. |
| k-anonymity | A privacy notion where each record is indistinguishable from at least k-1 others on quasi-identifiers. | It is a foundational privacy definition to compare against DP. |
| Quasi-identifier | An attribute or attribute combination that can help identify a person indirectly. | It explains how re-identification happens even without explicit IDs. |
| Differential Privacy (DP) | A formal privacy guarantee saying output changes only slightly when one input changes. | It is the main theoretical privacy framework in the PDF. |
| Privacy budget (epsilon) | The DP parameter controlling the privacy/utility balance. | You need it to interpret how strong a DP guarantee is. |
| Neighboring datasets | Datasets that differ by one individual or one record. | They are the core comparison pair in DP definitions. |
| Metric Differential Privacy | A relaxation of DP that uses a distance in a metric space. | It helps adapt DP ideas to embeddings and text representations. |
| Embedding model | A model that maps text into a vector space. | The privacy PDF uses embeddings to make text obfuscation tractable. |
| Noisy sampling | A DP-style strategy that samples replacement terms with privacy-controlled randomness. | It defines one family of text obfuscation mechanisms. |
| Noisy embeddings | A strategy that adds noise directly in embedding space. | It defines the other main family of mechanisms in the slides. |
| SanText | A text sanitization method based on DP-style noisy word replacement. | It is one of the named privacy methods explicitly covered. |
| Calibrated Multivariate Perturbations (CMP) | A method that adds calibrated multivariate noise to embeddings. | It is a core privacy mechanism discussed by name. |
| Mahalanobis distance | A distance measure that accounts for the covariance structure of the space. | It motivates a more geometry-aware privacy mechanism. |
| DP-COMET | A contextual DP obfuscation method for text using noisy contextual embeddings. | It is one of the most modern privacy mechanisms in the PDF. |

## Advanced Text Analytics and NLP

| Term | Meaning | Why it matters |
| --- | --- | --- |
| Advanced Text Analytics (ATA) | Going beyond keyword matching to extract structured meaning from text. | It is the focus of the ATA PDF. |
| Natural Language Processing (NLP) | Methods for processing and understanding human language computationally. | ATA and neural IR both rely on NLP ideas. |
| Semantic annotation | Attaching machine-readable meaning to spans of text. | It is one of the bridges from raw text to structured knowledge. |
| Sentiment analysis | Detecting opinions or polarity in text. | It is one of the first concrete ATA applications in the PDF. |
| Text classification | Assigning a document or passage to one or more categories. | It is a basic supervised NLP task used in many pipelines. |
| Information extraction | Pulling structured facts from unstructured text. | It is a major goal of ATA systems. |
| Named Entity Recognition (NER) | Detecting mentions of entities such as people, places, or organizations. | It is one of the most common first steps in information extraction. |
| Relation extraction | Identifying relations between entities mentioned in text. | It lets you move from entities to structured facts. |
| Question answering (QA) | Answering natural-language questions from text or knowledge sources. | It is a practical downstream application of text understanding. |
| Topic modeling | Discovering latent themes in a document collection. | It is useful for exploratory analysis of large corpora. |
| Specialized language | Domain-specific language used by experts. | ATA must often handle expert vocabulary, not just general language. |
| Terminology | The set of domain-specific terms and the concepts they denote. | It matters whenever text meaning depends on a field-specific lexicon. |
| Phrase | A group of words acting as a unit without a full subject-predicate structure. | The ATA PDF uses it to contrast shallow units with richer syntax. |
| Clause | A group of words with a subject-predicate structure. | It matters when moving beyond bag-of-words to syntactic structure. |

## Neural IR

| Term | Meaning | Why it matters |
| --- | --- | --- |
| Word embedding | A dense vector representation intended to capture semantic similarity. | It is the conceptual starting point of neural IR. |
| Distributional hypothesis | The idea that words used in similar contexts tend to have similar meanings. | It is the main intuition behind embeddings. |
| Static embeddings | Embeddings where each word has one fixed vector regardless of context. | They are the classical dense representation baseline. |
| Contextualized embeddings | Embeddings that change with surrounding context. | They power modern transformer-based retrieval models. |
| word2vec | A self-supervised embedding method based on context prediction. | It is the classic method everyone in neural IR should recognize. |
| fastText | An embedding method that includes subword information. | It helps handle morphology and rare words better than plain word2vec. |
| GloVe | An embedding method learned from global co-occurrence statistics. | It is one of the canonical static embedding approaches. |
| BERT | A transformer encoder producing bidirectional contextual representations. | It is the reference model behind many neural IR systems. |
| RoBERTa | A stronger training recipe and variant of BERT. | It is a standard contextual encoder in IR and NLP. |
| GPT | A transformer family trained autoregressively. | It is one of the major contextual representation families mentioned in the slides. |
| Cross-encoder | A model that jointly encodes query and document and scores them together. | It is powerful for re-ranking because it models direct interaction. |
| Bi-encoder / dual-encoder | A model that encodes query and document separately. | It enables fast retrieval in large collections. |
| Dense retrieval / neural retrieval | Retrieving documents through similarity between learned dense vectors. | It is the main neural alternative to lexical first-stage retrieval. |
| Knowledge distillation | Training a smaller or simpler model from stronger teacher signals. | It is a recurring technique in modern neural IR. |
| Approximate Nearest Neighbor (ANN) | A fast search strategy for large vector spaces that trades exactness for speed. | Dense retrieval is rarely practical at scale without ANN search. |
| SPLADE | A sparse neural retrieval model that expands and weights vocabulary terms. | It connects neural training with lexical first-stage ranking. |
| ANCE | A dense retrieval approach trained with hard negatives mined through ANN search. | It is one of the important named dense retrievers in the slides. |
| DPR | Dense Passage Retrieval, a dual-encoder approach for passage search. | It is a standard dense baseline to know. |
| DUET | A neural ranking model combining exact term matching with distributed matching. | It shows a hybrid view between lexical and neural signals. |
| KNRM | A neural ranking model using kernel pooling over term similarity signals. | It is a classic interaction-focused neural ranker. |
| C-KNRM | A convolutional extension of KNRM using n-gram matching. | It adds richer local matching patterns than plain KNRM. |
| PACRR | A neural re-ranking model using position-aware convolutions over similarity matrices. | It is another key interaction-focused model family. |
| TAS-B | A strong BERT-based bi-encoder baseline for dense retrieval. | It appears as a reference point in the sparse-vs-dense comparison material. |
| BEIR | A benchmark suite for zero-shot evaluation across heterogeneous IR datasets. | It is important for understanding generalization beyond a single benchmark. |
| Sparse encoder | A neural encoder that outputs weighted vocabulary dimensions instead of dense vectors. | It is the conceptual family behind SPLADE-like models. |
