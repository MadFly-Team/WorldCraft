# AI Prompt Playbook

This playbook is for AI-assisted game development where the AI produces as much implementation as possible under the guidance of an experienced game programmer.

## Goals

- Maximize useful AI output
- Keep prompts scoped and practical
- Reduce wasted usage on large unfocused sessions
- Preserve project structure and maintainability
- Use premium models only when the task justifies it

## Working Principles

1. **Plan first, code second**
2. **Keep tasks tightly scoped**
3. **Use premium models only for high-value work**
4. **Use completions for small/local changes**
5. **Prefer the smallest playable milestone**
6. **Avoid unrelated refactors**
7. **Start a new thread per feature/system**

## Model Strategy

### Use completions / next edit suggestions for
- boilerplate
- glue code
- repetitive local edits
- simple cleanup
- obvious implementations

### Use a default/lower-cost chat model for
- planning
- tracing code flow
- breaking features into tasks
- lightweight debugging
- test ideas
- small file rewrites

### Use Claude / premium model for
- multi-file features
- architecture and system design
- difficult debugging
- major refactors
- substantial code generation
- first-pass implementation of important systems

## Standard Feature Workflow

1. Trace or inspect the relevant system
2. Ask for a plan before asking for code
3. Approve scope, files, and risks
4. Implement in a bounded pass
5. Ask the AI to review its own output
6. Finish/polish locally with completions

## Prompt Templates

### 1) Plan First, No Code Yet

```text
Review this feature request and do not write code yet.

Feature request:
[describe feature]

I want:
- a short implementation plan
- the files likely involved
- key risks
- edge cases
- suggested order of work

Constraints:
- keep scope tight
- avoid unrelated refactors
- preserve current architecture
- identify what should not be changed

Return:
1. Summary
2. File impact
3. Plan
4. Risks
5. Suggested first implementation step
```

### 2) Bounded Implementation

```text
Implement the approved feature using the plan below.

Approved plan:
[paste plan]

Allowed files:
[list files]

Do not modify:
[list files / systems]

Requirements:
- Keep changes minimal and production-minded.
- Match existing style and patterns.
- Add comments only where they provide real value.
- If a missing dependency or design issue appears, stop and explain it clearly instead of guessing.

Return:
1. What changed
2. Why
3. Any assumptions made
4. Any follow-up work needed
```

### 3) Debugging Prompt

```text
Help me debug a game issue.

Problem:
[describe the bug]

Observed behavior:
[what happens]

Expected behavior:
[what should happen]

Relevant files / systems:
[list them]

Recent changes:
[list if known]

Please:
1. Identify the most likely causes in priority order.
2. Show the reasoning.
3. Suggest the smallest safe fix first.
4. Mention any logging/instrumentation I should add.
5. Avoid broad rewrites unless clearly justified.
```

### 4) Trace the Flow

```text
Trace how this behavior currently works through the codebase.

Behavior to trace:
[example: player takes damage and UI updates]

Start point:
[file/function/event if known]

I want:
1. the call/data flow
2. involved files and responsibilities
3. hidden dependencies
4. likely extension points
5. risks if I change it

Keep the explanation concise and structured.
Do not propose changes yet unless you spot an obvious defect.
```

### 5) Playable Milestone

```text
Define the smallest playable milestone for this feature.

Feature:
[description]

I want:
1. the minimum version that is actually playable/testable
2. what can be deferred
3. the implementation order
4. likely technical shortcuts acceptable for milestone one
5. what must be solid now to avoid painful rework later
```

### 6) Self-Review Prompt

```text
Review the implementation critically.

I want you to act like a senior engineer reviewing this change.

Check for:
- logic errors
- hidden assumptions
- edge-case failures
- poor naming
- architecture drift
- save/load/network/UI implications if relevant

Return:
1. Findings ordered by severity
2. Suggested fixes
3. Anything that should be deferred
4. Whether this is safe for a first playable version
```

### 7) Professional-Guided Experiment Prompt

```text
Treat this project as an experiment in AI-assisted game development.

Your job:
- generate as much of the implementation as reasonably possible
- but stay within the guidance of an experienced game programmer
- optimize for progress, coherence, and maintainability
- avoid flashy overengineering

For each substantial task:
1. summarize the task
2. propose a practical implementation
3. identify assumptions
4. identify where human review matters most
5. implement in the smallest sensible increment

Project principle:
We are testing how far AI can take a real game project under professional direction, so favor practical deliverables over theoretical perfection.
```

### 8) Cost-Control Prompt

```text
Answer as efficiently as possible.

Rules:
- keep the response concise
- only inspect files directly related to the request
- avoid broad rewrites
- avoid generating large amounts of code unless necessary
- propose a plan first if the task is unclear or large
- prefer the smallest working increment
```

## Recommended Session Pattern

For each feature:

1. Start with **Trace the Flow** or **Plan First**
2. Decide whether the task deserves a premium model
3. Use **Bounded Implementation** for the coding pass
4. Run **Self-Review Prompt** on the result
5. Finish with local edits and completions

## Good Habits

- Keep one thread per feature/system
- Put file boundaries in prompts
- Explicitly list what must not change
- Ask for risks before implementation
- Prefer incremental delivery over big rewrites
- Reuse prompt templates instead of improvising every request

## Avoid

- giant all-in-one prompts
- vague requests like "build the whole system"
- speculative refactors
- hidden scope expansion
- using premium models for trivial changes
- letting long chat threads accumulate too much context

## Short Version

If in doubt:
- ask for a plan first
- keep the scope narrow
- use premium models for hard/high-value work only
- aim for the smallest playable version
- finish small edits with completions
