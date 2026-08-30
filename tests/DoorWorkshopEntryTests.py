from pathlib import Path
root = Path(__file__).resolve().parents[1]
game = (root / 'src/Game.cpp').read_text(encoding='utf-8')
game_h = (root / 'src/Game.h').read_text(encoding='utf-8')
world = (root / 'src/World.cpp').read_text(encoding='utf-8')
world_h = (root / 'src/World.h').read_text(encoding='utf-8')
character = (root / 'src/Character.cpp').read_text(encoding='utf-8')
character_h = (root / 'src/Character.h').read_text(encoding='utf-8')

checks = {
    'door entry state machine': 'PersonnelDoorSequencePhase { None, Approach, Reach, TurnHandle, PullDoor, WalkInside }' in game_h,
    'outside interaction starts sequence': 'BeginPersonnelDoorEntrySequence();' in game,
    'walks to inside point': 'world.PersonnelDoorInsidePoint()' in game and 'PersonnelDoorSequencePhase::WalkInside' in game,
    'auto build on completed entry': 'workshopAutoBuildSession=true;' in game and 'EnterBuilderNow();' in game,
    'auto session ends outside': 'automatic Build Mode session ended' in game,
    'larger handle grip': 'DrawSphere(grip,0.145f' in world,
    'larger click target': 'p.x-0.30f' in world and 'p.x+0.30f' in world,
    'visible turning handle': 'doorHandleTurn*38.0f*DEG2RAD' in world,
    'handle exposes moving grip': 'PersonnelDoorHandleSpindlePosition' in world_h and 'SetPersonnelDoorHandleTurn' in world_h,
    'damped spring arm physics': 'const float stiffness=92.0f;' in character and 'const float damping=18.5f;' in character,
    'two bone arm IK': 'upperLen*upperLen-foreLen*foreLen+d*d' in character and 'ikElbow' in character,
    'hand follows live knob target': 'SetArmPhysicsTarget(true,world.PersonnelDoorHandlePosition()' in game,
    'equipment hides during grip': 'armConstraint.weight>0.15f' in character,
    'forced procedural walking': 'forceWalkAnimation' in character_h and 'if (forceWalkAnimation) animation.SetState(CharacterAnimState::Walk);' in character,
    'door threshold refreshes live collision cache': 'if(world.UpdatePersonnelDoor(dt,hangarInteraction.Door().openSpeed))' in game and 'collision.RefreshWorldGeometry(world);' in game,
    'scripted traversal ghosts personnel door': 'SetPersonnelDoorTraversalCollisionDisabled(true)' in game and '!doorTraversalCollisionDisabled' in world,
    'traversal collision is released after entry': 'SetPersonnelDoorTraversalCollisionDisabled(false)' in game,
}
for name, ok in checks.items():
    print(('[OK] ' if ok else '[FAIL] ') + name)
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('Door/workshop interaction regression checks failed: ' + ', '.join(failed))
print('Door + arm physics + automatic workshop entry regression checks passed.')
