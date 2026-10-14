# Autonomous Strategy Game Agent (C++)

An autonomous game-playing agent developed for the **Data Structures and Algorithms (EDA)** course at the **Facultat d'Informàtica de Barcelona (FIB-UPC)**.

## Competition Results

**Ranked 47th out of 335 participants — Top 14%**

The agent competed against other students' implementations in a university programming competition.

## Project Overview

The project consists of an autonomous agent designed for a turn-based, 2D grid-based strategy game.

The agent controls multiple units, navigates the game environment, collects resources, avoids threats, engages enemies, and solves combinatorial puzzles.

## Algorithms and Techniques

### Multi-source Breadth-First Search (BFS)

The agent computes distance maps from multiple starting positions to efficiently navigate towards:

- Money and resources
- Health and poison kits
- Special objective boxes
- Enemy soldiers

Distance maps are computed once per game turn and then reused for constant-time distance lookups.

### Backtracking

A recursive backtracking algorithm solves combinatorial puzzles involving groups of three numbers satisfying arithmetic constraints.

### Heuristic Decision-Making

Actions are selected using priority-based rules, considering:

- Nearby enemies
- Resource availability
- Unit health
- Distance to objectives
- Game progression

### Role-Based Strategies

During later stages of the game, soldiers follow different strategies:

- **Farmers:** prioritize collecting money.
- **Poisoners:** prioritize acquiring poison resources.
- **Attackers:** prioritize combat against enemy units.

## Technologies

- C++
- Standard Template Library (STL)
- Graph traversal algorithms
- Recursive backtracking
- Heuristic decision-making

## Project Structure

- `src/` — Autonomous agent implementation.

## Dependencies

The implementation depends on the original university-provided game engine, including `Player.hh`.

The engine is not distributed in this repository. Therefore, the agent cannot be compiled or executed independently without the original environment.

## Academic Context

Developed as part of the EDA course at FIB-UPC.

This repository preserves the original implementation as an example of algorithmic problem-solving and autonomous decision-making in C++.
