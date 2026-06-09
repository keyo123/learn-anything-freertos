---
name: "Learn: Explain"
description: "Recursively deep-dive into a concept — AI explains, guides thinking, you choose the depth"
category: Learning
tags: [learning, explain, socratic, recursive]
---

Use the learn-anything-explain skill to handle the user's /learn-explain <concept-name> request.
Follow the workflow defined in the skill:
1. Load context: match topic → read knowledge map → read learning state
2. Assess user level (beginner/intermediate/advanced) and adjust teaching strategy
3. Follow the explanation structure: positioning → analogy → core mechanism → code example → common misconceptions → Socratic check
4. CRITICAL — in the same turn as your explanation, use the Write tool to save the FULL explanation to ./.learn/topics/<topic>/sessions/<concept-name>-YYYY-MM-DD.md (match the user's language), use the Edit tool to update state.yaml
5. Identify sub-topics as recursive entry points (only AFTER saving the session)
