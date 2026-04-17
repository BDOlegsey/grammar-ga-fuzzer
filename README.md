# Adaptive Grammar Fuzzing with Genetic Algorithms

This repository contains a research implementation of an adaptive grammar-based fuzzing approach, where production rule probabilities of a context-free grammar are optimized using a genetic algorithm (GA).

The goal is to improve fuzzing efficiency by dynamically adjusting input generation strategies based on feedback from the target program.

---

## 🚀 Key Idea

Traditional grammar-based fuzzers rely on **static probabilities** when selecting grammar rules. This often leads to inefficient exploration of the input space.

This project introduces:

* Adaptive probability tuning
* Feedback-driven input generation
* Genetic optimization of grammar rule weights

---

## 🧠 Method Overview

We model fuzzing as an optimization problem:

* Each individual = a set of probabilities for grammar rules
* Fitness = combination of:

  * Code coverage
  * Number of discovered bugs

### Fitness Function

F = α · Coverage + β · Bugs

Where:

* α, β — weighting coefficients

---

## ⚙️ System Architecture

Pipeline:

Grammar → Generator → Fuzzer → Coverage Analyzer → Genetic Algorithm → Updated Probabilities

---

## 🧬 Genetic Algorithm

The GA operates as follows:

1. Initialize population of probability distributions
2. Evaluate fitness for each individual
3. Select individuals (tournament selection)
4. Apply crossover
5. Apply mutation
6. Normalize probabilities
7. Repeat until convergence

---

## 🧪 Experiment Setup

* Grammar: JSON-like context-free grammar
* Population size: 50
* Generations: 100
* Mutation rate: 0.1
* Crossover rate: 0.7
* Runs: 10 (with fixed random seeds)

### Baseline

Static grammar fuzzing with uniform rule probabilities.

---

## 📊 Results

The adaptive approach shows:

* Increased code coverage
* Higher bug discovery rate
* More efficient exploration of input space

---

## 📂 Project Structure

```
.
├── src/
│   ├── grammar.cpp
│   ├── generator.cpp
│   ├── ga.cpp
│   ├── fitness.cpp
│   └── main.cpp
├── include/
├── experiments/
├── results/
└── README.md
```

---

## 🛠️ Build

```bash
g++ -std=c++17 -O2 src/*.cpp -o fuzzer
```

---

## ▶️ Run

```bash
./fuzzer
```

---

## 📌 Notes on Reproducibility

* Fixed random seed is used
* Multiple runs are averaged
* Results include statistical metrics (mean, std)

---

## 📖 Research Context

This project is based on the idea that:

> Adaptive grammars outperform static ones in fuzzing scenarios by leveraging feedback from execution.

---

## ⚠️ Limitations

* Simplified fitness evaluation (placeholders for coverage/bugs)
* Single grammar (JSON)
* No parallel execution

---

## 🔮 Future Work

* Integration with real coverage tools (e.g., LLVM Sanitizers)
* Multi-objective optimization
* Support for multiple grammars
* Reinforcement learning alternatives

---

## 📜 License

MIT License
