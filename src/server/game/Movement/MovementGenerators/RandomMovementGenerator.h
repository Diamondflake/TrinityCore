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

#ifndef TRINITY_RANDOMMOTIONGENERATOR_H
#define TRINITY_RANDOMMOTIONGENERATOR_H

#define DONT_CACHE_RANDOM_MOVEMENT_PATHS    0
// With uint_8 the maximum number of movement points is 15 (i.e. 16 * 15 paths due to the origin)
#define RANDOM_MOVEMENT_POINTS              5

#define PAUSE_ODDS                    0.5f
#define PAUSE_MINIMUM_DURATION_MS     200
#define PAUSE_MAXIMUM_DURATION_MS     800

#include "MovementGenerator.h"
#include "Position.h"
#include "Timer.h"

class PathGenerator;

template<class T>
class RandomMovementGenerator : public MovementGeneratorMedium<T, RandomMovementGenerator<T>>
{
    public:
        explicit RandomMovementGenerator(float distance = 0.0f);

        MovementGeneratorType GetMovementGeneratorType() const override;

        void Pause(uint32 timer = 0) override;
        void Resume(uint32 overrideTimer = 0) override;

        void DoInitialize(T*);
        void DoReset(T*);
        bool DoUpdate(T*, uint32);
        void DoDeactivate(T*);
        void DoFinalize(T*, bool, bool);

        void UnitSpeedChanged() override { RandomMovementGenerator<T>::AddFlag(MOVEMENTGENERATOR_FLAG_SPEED_UPDATE_PENDING); }

    private:
        void SetRandomLocation(T*);

        TimeTracker _timer;
        Position _reference;
        float _wanderDistance;
        uint8 _wanderSteps;

    #if DONT_CACHE_RANDOM_MOVEMENT_PATHS == 0

        std::unique_ptr<PathGenerator> _paths[(RANDOM_MOVEMENT_POINTS + 1) * RANDOM_MOVEMENT_POINTS];
        // With uint_8 the maximum number of movement points is 15 (i.e. 16 * 15 paths due to the origin)
        // Max value is reserved for "no path"
        uint_8 _currentPathIndex;

    #else

        std::unique_ptr<PathGenerator> _path;

    #endif
};

#endif
