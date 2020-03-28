# Horde-Public-release-
Horde - an AGP interpreter 

"Horde" is an interpreter for Asynchronous Graph Programming (AGP) code. AGP is a programming paradigm for asynchronous, parallelizeable computation.

AGP is currently implemented as an embedded language that must be compiled into the Horde binary. Since Horde was developed simultaneously with AGP design, there is currently some "dead code" due to redesign and improvements. This will be addressed in future releases. Horde successfully interpreted our test cases, but we have not performed extensive testing: bugs are likely to exist. We will address them continuously in the upcoming months. AGP code is not programmer-friendly: it is not meant to be written by humans, but rather to be used as an Intermediate Representation by higher level languages - we will soon begin development of a compiler that outputs AGP code. Code generation (primarily targetting bare-metal deployment) will also be available in the future.

The Horde interpreter accompanies the paper "Towards a Programming Paradigm for Reconfigurable Computing: Asynchronous Graph Programming", currently under consideration at IEEE ETFA 2020.
