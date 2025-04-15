/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RandomMovementGenerator.h"
#include "Creature.h"
#include "Map.h"
#include "MovementDefines.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "PathGenerator.h"
#include "Random.h"

template<class T>
RandomMovementGenerator<T>::RandomMovementGenerator(float distance) : _timer(0), _reference(), _wanderDistance(distance), _wanderSteps(0)
{
    this->Mode = MOTION_MODE_DEFAULT;
    this->Priority = MOTION_PRIORITY_NORMAL;
    this->Flags = MOVEMENTGENERATOR_FLAG_INITIALIZATION_PENDING;
    this->BaseUnitState = UNIT_STATE_ROAMING;
}

template RandomMovementGenerator<Creature>::RandomMovementGenerator(float/* distance*/);

template<class T>
MovementGeneratorType RandomMovementGenerator<T>::GetMovementGeneratorType() const
{
    return RANDOM_MOTION_TYPE;
}

template<class T>
void RandomMovementGenerator<T>::Pause(uint32 timer /*= 0*/)
{
    if (timer)
    {
        this->AddFlag(MOVEMENTGENERATOR_FLAG_TIMED_PAUSED);
        _timer.Reset(timer);
        this->RemoveFlag(MOVEMENTGENERATOR_FLAG_PAUSED);
    }
    else
    {
        this->AddFlag(MOVEMENTGENERATOR_FLAG_PAUSED);
        this->RemoveFlag(MOVEMENTGENERATOR_FLAG_TIMED_PAUSED);
    }
}

template<class T>
void RandomMovementGenerator<T>::Resume(uint32 overrideTimer /*= 0*/)
{
    if (overrideTimer)
        _timer.Reset(overrideTimer);

    this->RemoveFlag(MOVEMENTGENERATOR_FLAG_PAUSED);
}

template MovementGeneratorType RandomMovementGenerator<Creature>::GetMovementGeneratorType() const;

template<class T>
void RandomMovementGenerator<T>::DoInitialize(T*) { }

template<>
void RandomMovementGenerator<Creature>::DoInitialize(Creature* owner)
{
    RemoveFlag(MOVEMENTGENERATOR_FLAG_INITIALIZATION_PENDING | MOVEMENTGENERATOR_FLAG_TRANSITORY | MOVEMENTGENERATOR_FLAG_DEACTIVATED | MOVEMENTGENERATOR_FLAG_TIMED_PAUSED);
    AddFlag(MOVEMENTGENERATOR_FLAG_INITIALIZED);

    if (!owner || !owner->IsAlive())
        return;

    _reference = owner->GetPosition();
    owner->StopMoving();

    if (_wanderDistance == 0.f)
        _wanderDistance = owner->GetWanderDistance();

#if DONT_CACHE_RANDOM_MOVEMENT_PATHS == 0

    // Generate points
    // Origin included as the first point to be used for checks
    Position points[RANDOM_MOVEMENT_POINTS + 1];
    points[0] = _reference;

    uint8 i = 1; // point being generated
    uint8 j; // point being used for checks
    uint8 attempts;
    Position position;
    bool acceptable;
    std::unique_ptr<PathGenerator> path;
    while (i < RANDOM_MOVEMENT_POINTS + 1)) {

        position = position(_reference);
        float distance = frand(0.f, _wanderDistance * 2); // The * 2 accounts for the distance being halved immediately
        float angle = frand(0.f, float(M_PI * 2));

        acceptable = false;
        attempts = 0;
        while (!acceptable) {
            // Number of attempts is theoretically not needed but mght be, or be an alternative to the distance halving
            // if it proves too much of an issue because it creates many points near the reference in some cases
            /*if (attempts = TRIES_PER_RANDOM_POINTS) {
                points[i] = points[i-1]; // exists because points[0] is the reference
                i++;
            }*/
            
            // If the position is not suitable, instead of retrying the entire process, distance is halved
            // to eventually find a suitable position (since the current position of the mob is being approached)
            // this should prevent awkward mob placement (e.g. right against a wall) from leading to too many tries
            distance /= 2.f;
            position = position(_reference); // needed ? Memory leak ?
            owner->MovePositionToFirstCollision(position, distance, angle);

            acceptable = true;
            for (j = 0; j < i; j++) {
                if (!owner->IsWithinLOS(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ())) {
    
                    acceptable = false;
                    //attempts++;
                    break;
                }
            }

            // Paths are checked after LoS to avoid expensive path checks when LoS isn't guaranteed already
            if (acceptable == true) {
                for (j = 0; j < i; j++) {
                    // TODO: replace with path check

                    path = std::make_unique<PathGenerator>(owner);
                    path->SetPathLengthLimit(30.0f);

                    // TODO: directly set paths so they don't have to be recalculated in case of success
                
                    bool result = path->CalculatePath(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ());
                    // PATHFIND_FARFROMPOLY shouldn't be checked as creatures in water are most likely far from poly
                    if (!result || (path->GetPathType() & PATHFIND_NOPATH)
                                || (path->GetPathType() & PATHFIND_SHORTCUT)
                                /*|| (_path->GetPathType() & PATHFIND_FARFROMPOLY)*/)
                    {
                        acceptable = false;
                        //attempts++;
                        break;
                    }
                }
            }
        }
        
        points[i] = position;
        i++;
    }


    // All paths must exist ! Need to do iteratively as points are created
    // Generates paths between the chosen points

    

    // i is the point currently being set; Several loops may be made with the same i
    

#endif

    // Retail seems to let a creature walk 2 up to 10 splines before triggering a pause
    _wanderSteps = urand(1, ((_wanderDistance <= 1.0f) ? 2 : 8));

    _timer.Reset(0);

#if DONT_CACHE_RANDOM_MOVEMENT_PATHS == 1
    _path = nullptr;
#endif
}

template<class T>
void RandomMovementGenerator<T>::DoReset(T*) { }

template<>
void RandomMovementGenerator<Creature>::DoReset(Creature* owner)
{
    RemoveFlag(MOVEMENTGENERATOR_FLAG_TRANSITORY | MOVEMENTGENERATOR_FLAG_DEACTIVATED);

    DoInitialize(owner);
}

template<class T>
void RandomMovementGenerator<T>::SetRandomLocation(T*) { }

template<>
void RandomMovementGenerator<Creature>::SetRandomLocation(Creature* owner)
{
    if (!owner)
        return;

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE | UNIT_STATE_LOST_CONTROL) || owner->IsMovementPreventedByCasting())
    {
        AddFlag(MOVEMENTGENERATOR_FLAG_INTERRUPTED);
        owner->StopMoving();
#if DONT_CACHE_RANDOM_MOVEMENT_PATHS == 1
        _path = nullptr;
#endif
        return;
    }

#if DONT_CACHE_RANDOM_MOVEMENT_PATHS == 1

    Position position(_reference);
    float distance = frand(0.f, _wanderDistance);
    float angle = frand(0.f, float(M_PI * 2));
    owner->MovePositionToFirstCollision(position, distance, angle);

    // Check if the destination is in LOS
    if (!owner->IsWithinLOS(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ()))
    {
        // Retry later on
        _timer.Reset(200);
        return;
    }

    if (!_path)
    {
        _path = std::make_unique<PathGenerator>(owner);
        _path->SetPathLengthLimit(30.0f);
    }

    bool result = _path->CalculatePath(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ());
    // PATHFIND_FARFROMPOLY shouldn't be checked as creatures in water are most likely far from poly
    if (!result || (_path->GetPathType() & PATHFIND_NOPATH)
                || (_path->GetPathType() & PATHFIND_SHORTCUT)
                /*|| (_path->GetPathType() & PATHFIND_FARFROMPOLY)*/)
    {
        _timer.Reset(100);
        return;
    }

#else

    // Because we cannot fail to find a new path, we simulate failures 
    // to have pauses within the movement
    if (frand(0.f, 1.f) <= PAUSE_ODDS) {
        _timer.Reset(urand(PAUSE_MINIMUM_DURATION_MS, PAUSE_MAXIMUM_DURATION_MS));
    }

    // There are RANDOM_MOVEMENT_POINTS other points because the _reference is included
    // (_currentPathIndex / RANDOM_MOVEMENT_POINTS) = current end point and next starting point
    _currentPathIndex = (_currentPathIndex / RANDOM_MOVEMENT_POINTS) * RANDOM_MOVEMENT_POINTS + urand(0, RANDOM_MOVEMENT_POINTS - 1);

#endif

    RemoveFlag(MOVEMENTGENERATOR_FLAG_TRANSITORY | MOVEMENTGENERATOR_FLAG_TIMED_PAUSED);

    owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);

    bool walk = true;
    switch (owner->GetMovementTemplate().GetRandom())
    {
        case CreatureRandomMovementType::CanRun:
            walk = owner->IsWalking();
            break;
        case CreatureRandomMovementType::AlwaysRun:
            walk = false;
            break;
        default:
            break;
    }

    Movement::MoveSplineInit init(owner);
    
#if DONT_CACHE_RANDOM_MOVEMENT_PATHS == 1
    init.MovebyPath(_path->GetPath());
#else
    init.MovebyPath(_paths[_currentPathIndex]->GetPath());
#endif
    
    init.SetWalk(walk);
    int32 splineDuration = init.Launch();

    --_wanderSteps;
    if (_wanderSteps) // Creature has yet to do steps before pausing
        _timer.Reset(splineDuration);
    else
    {
        // Creature has made all its steps, time for a little break
        _timer.Reset(splineDuration + urand(6, 12) * IN_MILLISECONDS); // Retails seems to use rounded numbers so we do as well
        _wanderSteps = urand(1, ((_wanderDistance <= 1.0f) ? 2 : 8));
    }

    // Call for creature group update
    owner->SignalFormationMovement();
}

template<class T>
bool RandomMovementGenerator<T>::DoUpdate(T*, uint32)
{
    return false;
}

template<>
bool RandomMovementGenerator<Creature>::DoUpdate(Creature* owner, uint32 diff)
{
    if (!owner || !owner->IsAlive())
        return true;

    if (HasFlag(MOVEMENTGENERATOR_FLAG_FINALIZED | MOVEMENTGENERATOR_FLAG_PAUSED))
        return true;

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
    {
        AddFlag(MOVEMENTGENERATOR_FLAG_INTERRUPTED);
        owner->StopMoving();
#if DONT_CACHE_RANDOM_MOVEMENT_PATHS == 1
        _path = nullptr;
#endif
        return true;
    }
    else
        RemoveFlag(MOVEMENTGENERATOR_FLAG_INTERRUPTED);

    _timer.Update(diff);

    if ((HasFlag(MOVEMENTGENERATOR_FLAG_SPEED_UPDATE_PENDING) && !owner->movespline->Finalized()) || (_timer.Passed() && owner->movespline->Finalized()))
        SetRandomLocation(owner);

    return true;
}

template<class T>
void RandomMovementGenerator<T>::DoDeactivate(T*) { }

template<>
void RandomMovementGenerator<Creature>::DoDeactivate(Creature* owner)
{
    AddFlag(MOVEMENTGENERATOR_FLAG_DEACTIVATED);
    owner->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
}

template<class T>
void RandomMovementGenerator<T>::DoFinalize(T*, bool, bool) { }

template<>
void RandomMovementGenerator<Creature>::DoFinalize(Creature* owner, bool active, bool/* movementInform*/)
{
    AddFlag(MOVEMENTGENERATOR_FLAG_FINALIZED);
    if (active)
    {
        owner->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
        owner->StopMoving();

        // TODO: Research if this modification is needed, which most likely isnt
        owner->SetWalk(false);
    }
}
