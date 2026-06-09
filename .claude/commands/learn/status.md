---
name: "Learn: Status"
description: Visualize learning state — knowledge map heatmap with mastery status per concept
category: Learning
tags: [learning, status, visualization]
---

Use the learn-anything-status skill to handle the user's /learn-status [topic-name] request.
Follow the workflow defined in the skill:
1. Determine topic (specified/single/multiple/none)
2. Read knowledge-map.md and state.yaml
3. Render heatmap following knowledge map structure, annotating status icons, practice count, confidence
4. Show summary panel: mastery stats, last practice, days learning
