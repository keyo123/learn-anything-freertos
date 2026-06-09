---
name: "Learn: Topic"
description: "Initialize or load a learning topic — view knowledge map, track progress, choose your path"
category: Learning
tags: [learning, topic, initialize]
---

Use the learn-anything-topic skill to handle the user's /learn <topic-name> request.
Follow the workflow defined in the skill:
1. Determine if the topic exists
2. New topic: create directory structure → generate knowledge-map.md and state.yaml → present knowledge map and guide the user
3. Existing topic: read data → calculate progress → give personalized recommendations
