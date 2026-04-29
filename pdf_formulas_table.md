# Formula Table from All PDFs

This sheet consolidates the standalone formulas found across all PDFs in this folder. I normalized notation where the slides used image-based equations or OCR-unfriendly symbols, so the expressions below are written in clean, readable mathematical form.

## Formula Table

| Area | Formula | Expression | Source PDF(s) | Notes |
| --- | --- | --- | --- | --- |
| Evaluation | Precision | `P = |Rel intersect Ret| / |Ret|` | `05-se-2025-26-measures.pdf` | Fraction of retrieved documents that are relevant. |
| Evaluation | Recall | `R = |Rel intersect Ret| / |Rel|` | `05-se-2025-26-measures.pdf` | Fraction of relevant documents that are retrieved. |
| Evaluation | F1-score | `F1 = 2PR / (P + R)` | `05-se-2025-26-measures.pdf` | Harmonic mean of precision and recall. |
| Evaluation | Precision at k | `P@k = (# relevant documents in the top k) / k` | `05-se-2025-26-measures.pdf` | Rank-based binary relevance measure. |
| Evaluation | Recall at k | `R@k = (# relevant documents in the top k) / |Rel|` | `05-se-2025-26-measures.pdf` | Recall computed at a rank cutoff. |
| Evaluation | R-Precision | `R-Prec = (# relevant documents in the top R ranks) / R`, with `R = |Rel|` | `05-se-2025-26-measures.pdf` | Uses the number of relevant documents as the cutoff. |
| Evaluation | Average Precision | `AP = (1 / |Rel|) * sum_(i=1..N) (P@i * rel_i)` | `05-se-2025-26-measures.pdf` | `rel_i` is `1` if the document at rank `i` is relevant, else `0`. |
| Evaluation | Mean Average Precision | `MAP = (1 / |Q|) * sum_q AP(q)` | `05-se-2025-26-measures.pdf` | Mean AP over a set of queries. |
| Evaluation | Reciprocal Rank | `RR = 1 / rank(first relevant document)` | `05-se-2025-26-measures.pdf`, `09-se-2025-26-neural-IR-model.pdf` | Used when only the first relevant hit matters. |
| Evaluation | Mean Reciprocal Rank | `MRR = (1 / |Q|) * sum_q RR(q)` | `05-se-2025-26-measures.pdf`, `09-se-2025-26-neural-IR-model.pdf` | Mean RR over multiple queries. |
| Evaluation | Discounted Cumulative Gain | `DCG@k = sum_(i=1..k) (2^(rel_i) - 1) / log2(i + 1)` | `03-se-2025-26-IR-basics.pdf`, `05-se-2025-26-measures.pdf` | Supports graded relevance. |
| Evaluation | Normalized DCG | `nDCG@k = DCG@k / IDCG@k` | `05-se-2025-26-measures.pdf`, `09-se-2025-26-neural-IR-model.pdf` | `IDCG` is the ideal DCG at the same cutoff. |
| Evaluation | Rank-Biased Precision | `RBP = (1 - p) * sum_(i=1..N) rel_i * p^(i-1)` | `05-se-2025-26-measures.pdf` | `p` is the user persistence parameter. |
| Traditional IR | Zipf's Law | `P_r ~= c / r^alpha` | `06-se-2025-26-traditional-IR-models.pdf` | Frequency/probability decreases with rank. |
| Traditional IR | Relative Term Frequency | `tf(t,d) = f_(t,d) / sum_(t') f_(t',d)` | `06-se-2025-26-traditional-IR-models.pdf` | Length-normalized term frequency. |
| Traditional IR | Log Term Frequency | `tf(t,d) = 1 + log2(f_(t,d))` if `f_(t,d) > 0`, else `0` | `06-se-2025-26-traditional-IR-models.pdf` | Dampens the effect of very frequent terms. |
| Traditional IR | Inverse Document Frequency | `idf(t) = log(N / n_t)` | `06-se-2025-26-traditional-IR-models.pdf` | `N` is the number of documents and `n_t` is the document frequency of term `t`. |
| Traditional IR | TF-IDF weight | `w(t,d) = tf(t,d) * idf(t)` | `06-se-2025-26-traditional-IR-models.pdf` | Standard term weighting scheme. |
| Traditional IR | Cosine Similarity | `cos(q,d) = (q . d) / (||q|| * ||d||)` | `06-se-2025-26-traditional-IR-models.pdf`, `09-se-2025-26-neural-IR-model.pdf` | Also used in embedding-based similarity. |
| Traditional IR | Rocchio Relevance Feedback | `q' = alpha*q + (beta / |Rel|) * sum_(d in Rel) d - (gamma / |NonRel|) * sum_(d in NonRel) d` | `06-se-2025-26-traditional-IR-models.pdf` | Updates the query vector using judged relevant and non-relevant documents. |
| Traditional IR | BM25 IDF | `IDF_BM25(t) = log((N - n_t + 0.5) / (n_t + 0.5))` | `06-se-2025-26-traditional-IR-models.pdf` | Common BM25 inverse document frequency term. |
| Traditional IR | BM25 (full form) | `BM25(d,q) = sum_(t in q) IDF(t) * ((f_(t,d) * (k1 + 1)) / (f_(t,d) + k1 * (1 - b + b * |d| / avgdl))) * ((f_(t,q) * (k3 + 1)) / (f_(t,q) + k3))` | `06-se-2025-26-traditional-IR-models.pdf` | Includes document and query term-frequency factors. |
| Traditional IR | BM25 (short-query form) | `BM25(d,q) = sum_(t in q) IDF(t) * ((f_(t,d) * (k1 + 1)) / (f_(t,d) + k1 * (1 - b + b * |d| / avgdl)))` | `06-se-2025-26-traditional-IR-models.pdf` | Common simplification when query term weights are not expanded. |
| Traditional IR | Query Likelihood | `P(Q | theta_D) = product_(w in Q) P(w | theta_D)^(f_(w,Q))` | `06-se-2025-26-traditional-IR-models.pdf` | Scores how likely the query is under the document language model. |
| Traditional IR | Maximum Likelihood Estimate | `P(w | theta_D) = f_(w,D) / |D|` | `06-se-2025-26-traditional-IR-models.pdf` | Basic document language-model estimate. |
| Traditional IR | Jelinek-Mercer Smoothing | `P(w | theta_hat_D) = (1 - lambda) * (f_(w,D) / |D|) + lambda * (f_(w,C) / |C|)` | `06-se-2025-26-traditional-IR-models.pdf` | Interpolates the document model with the collection model. |
| Traditional IR | Dirichlet Smoothing | `P(w | theta_hat_D) = (f_(w,D) + mu * (f_(w,C) / |C|)) / (|D| + mu)` | `06-se-2025-26-traditional-IR-models.pdf` | Adds pseudo-counts from the collection model. |
| Privacy | Differential Privacy | `Pr[M(D) in S] <= exp(epsilon) * Pr[M(D') in S]` | `07-se-2025-26-privacy.pdf` | `D` and `D'` are neighboring datasets. |
| Privacy | Metric Differential Privacy | `Pr[M(x) = z] <= exp(epsilon * d(x,x')) * Pr[M(x') = z]` | `07-se-2025-26-privacy.pdf` | Generalizes DP using a distance function over the input space. |
| Privacy | Euclidean Distance | `d(x,y) = sqrt(sum_i (x_i - y_i)^2)` | `07-se-2025-26-privacy.pdf` | Shown as a baseline distance notion before Mahalanobis distance. |
| Privacy | Jaccard Similarity | `J(A,B) = |A intersect B| / |A union B|` | `07-se-2025-26-privacy.pdf` | Used for privacy/effectiveness evaluation of text obfuscation. |
| Privacy | Mahalanobis Distance | `d_M(x,y) = sqrt((x-y)^T * Sigma^(-1) * (x-y))` | `07-se-2025-26-privacy.pdf` | Accounts for covariance in the embedding space. |
| Privacy | Noisy Embedding Step | `e_noisy = e + eta`, with `eta ~ p_epsilon` | `07-se-2025-26-privacy.pdf` | Used in DP-COMET style contextual obfuscation. |
| Neural IR | SPLADE token weighting | `w_j = sum_(i in t) log(1 + ReLU(w_(i,j)))` | `09-se-2025-26-neural-IR-model.pdf` | Aggregates token-level activations into a sparse lexical weight. |
| Administration | Lateness penalty | `penalty(d) = max(0, 0.5 * ln(d))` | `01-se-2025-26-organisation.pdf` | Administrative formula from the course logistics slides; `d` is delay in days. |

## Notation

- `Rel`: set of relevant documents
- `Ret`: set of retrieved documents
- `Q`: query
- `q, d`: query vector and document vector
- `f_(t,d)`: frequency of term `t` in document `d`
- `f_(t,q)`: frequency of term `t` in query `q`
- `n_t`: number of documents containing term `t`
- `N`: number of documents in the collection
- `|d|`: document length
- `avgdl`: average document length
- `theta_D`: document language model
- `theta_hat_D`: smoothed document language model
- `C`: collection
- `lambda, mu, alpha, beta, gamma, epsilon`: model parameters

## Coverage Notes

- `01-se-2025-26-organisation.pdf`: contains the administrative lateness-penalty formula.
- `02-se-2025-26-introduction.pdf`: no standalone mathematical formulas worth extracting into a formula sheet.
- `03-se-2025-26-IR-basics.pdf`: mainly conceptual material plus a DCG-based evaluation example.
- `04-se-2025-26-IR-basics-handson.pdf`: mostly tools, encodings, and practical exercises; no additional standalone formulas beyond the IR metrics already listed.
- `05-se-2025-26-measures.pdf`: main source for evaluation formulas.
- `06-se-2025-26-traditional-IR-models.pdf`: main source for classical IR scoring and language-model formulas.
- `07-se-2025-26-privacy.pdf`: main source for privacy and distance formulas.
- `08-se-2025-26-ATA.pdf`: mostly conceptual/NLP pipeline content; no additional standalone mathematical formulas.
- `09-se-2025-26-neural-IR-model.pdf`: reuses cosine and ranking metrics, and adds the SPLADE weighting formula.
