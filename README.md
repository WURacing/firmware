# firmware

This repository contains all the embedded software written for our custom circuit boards on the racecar.
It is organized by platform: e.g. 2020 means WUFR-20, 2018 means BFR-18, etc.
Model years before the present should be considered to be frozen, unless that year's car is scheduled for further testing.

Branching model:
- master: Code for the race car that is ready for any testing event. All commits merging into master should be production-ready (validated) and tagged with the appropriate JIRA ticket. (e.g. with a message like "WUFR-999999: Add wheel speed support")
- connor/fix-some-bug: User branch for some work-in-progress change. Create as many as you need, but try to get them merged or deleted within a couple months.
- feature/stress-testing: Feature branch for limited test project, not intended to go into a car. Merge from a user branch.
