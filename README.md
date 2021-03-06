# Advanced_Computer_Architecture

<ins>Machine Problem 1: Implementing Cache Simulator</ins>  
- Machine Problem Description:
In this machine problem, a flexible cache and memory hierarchy simulator has been implemented and  the performance, area, and energy of different memory hierarchy configurations were compared, using a subset of the SPEC-2000 benchmark suite. 

- Specification of Memory Hierarchy:  
Designed a generic cache module that can be used at any level in a memory hierarchy. For example, this cache module can be “instantiated” as an L1 cache, an L2 cache, an L3 cache, and so on. 
  
<ins>Machine Problem 2: Implementing Branch Predictor</ins>    
- Machine Problem Description: In this machine problem, branch predictors well suited to the SPECint95 benchmarks were constructed
- List of Implemented Branch Predictors is given below:
1. Smith n-bit Counter Branch Predictor
2. GShare Branch Predictors (n=0: bimodal branch predictor, n>0: gshare branch predictor)
3. Hybrid Branch Predictor

<ins>Machine Problem 3: Dynamic Instruction Scheduling</ins>   
  
Machine Problem Description: In this project, a simulator for an out-of-order superscalar processor based on 
Tomasulo’s algorithm that fetches, dispatches, and issues N instructions per cycle was constructed. Only the 
dynamic scheduling mechanism has been modeled in detail, i.e., perfect caches and perfect branch 
prediction are assumed. 
 
