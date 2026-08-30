# v26.8.4 validation

Expected collision invariants:

1. Closed personnel door is solid.
2. Scripted traversal can disable the door slab immediately.
3. Personnel-door header only collides when the actor is vertically high enough to overlap it.
4. Open personnel doorway is passable in DEV and release builds.
5. Open main garage is passable under its high header/lintel.
6. Normal walls remain solid.
7. DEV noclip remains a separate developer-only bypass and is not required for normal doorway traversal.
