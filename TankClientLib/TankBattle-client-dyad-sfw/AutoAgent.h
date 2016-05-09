#pragma once

#include "Vector2.h"
#include <iostream>
#include "Graph.h"
#include "Solver.h"
#include "Grid.h"

#include "IAgent.h"
#include "sfwdraw.h"

/*
    Example dumb agent.

    State Description:
        

        Cannon:
            SCAN:
                Turns Right
                enemy in sight? -> Set Target, FIRE
            FIRE:
                Fire weapon
                -> SCAN

        Tank:
            Explore:
                If touching Target, set random Target
                Path to Target                
*/

class AutoAgent : public IAgent 
{
    // Cache current battle data on each update
    tankNet::TankBattleStateData current;
	tankNet::TankTacticalInfo info;
    // output data that we will return at end of update
    tankNet::TankBattleCommand tbc;

    // Could use this to keep track of the environment, check out the header.
    Grid map;



    //////////////////////
    // States for turret and tank
    enum TURRET { SCAN, AIM, FIRE } turretState = SCAN;
	enum TANK { SEEK, AVOID, CHASE } tankState = SEEK;

    float randTimer = 0;
	float prevhp;
	float prepos;
    // Active target location to pursue
    Vector2 target;
	Vector2 target1;
    ///////
    // Scanning
    void scan()
    {
        Vector2 tf = Vector2::fromXZ(current.cannonForward);  // current forward
        Vector2 cp = Vector2::fromXZ(current.position); // current position

        tbc.fireWish = false;
        // Arbitrarily look around for an enemy.
        tbc.cannonMove = tankNet::CannonMovementOptions::RIGHT;

        // Search through the possible enemy targets
        for (int aimTarget = 0; aimTarget < current.playerCount - 1; ++aimTarget)
            if (current.tacticoolData[aimTarget].inSight && current.canFire) // if in Sight and we can fire
            {
                target = Vector2::fromXZ(current.tacticoolData[aimTarget].lastKnownPosition);
             
                if(dot(tf, normal(target-cp)) > .9f)
                    turretState = FIRE;
                break;
            }
    }
	void aim()
	{
		Vector2 tf = Vector2::fromXZ(current.cannonForward);  // current forward
		Vector2 cp = Vector2::fromXZ(current.position); // current position
		tbc.fireWish = false;
		for (int aimTarget = 0; aimTarget < current.playerCount - 1; ++aimTarget)
			if (current.tacticoolData[aimTarget].inSight && current.canFire) // if in Sight and we can fire
			{
				target = Vector2::fromXZ(current.tacticoolData[aimTarget].lastKnownPosition);

				if (dot(tf, normal(target - cp)) < .0f)
				{
					tbc.cannonMove = tankNet::CannonMovementOptions::LEFT;
					

				}
				else if (dot(tf, normal(target - cp)) > .0f)
				{
					tbc.cannonMove = tankNet::CannonMovementOptions::RIGHT;
					

				}
				else
				{
					tbc.cannonMove = tankNet::CannonMovementOptions::HALT;
				}
				if (current.tacticoolData[aimTarget].inSight && current.canFire)
				{
					turretState = FIRE;
				}
			}
	}
    void fire()
    {
        // No special logic for now, just shoot.
        tbc.fireWish = current.canFire;
        turretState = AIM;
    }

    std::list<Vector2> path;
	void seek()
	{
		if (current.currentHealth == 100)
		{
			prevhp = 0;
			prevhp = current.currentHealth - 1;
		}


		if (current.currentHealth < prevhp)
		{
			prevhp = current.currentHealth - 1;
			tankState = AVOID;
		}
		{
			Vector2 cp = Vector2::fromXZ(current.position); // current position
			Vector2 cf = Vector2::fromXZ(current.forward);  // current forward


			randTimer -= sfw::getDeltaTime();

			// If we're pretty close to the target, get a new target           
			if (distance(target, cp) < 20 || randTimer < 0)
			{
				target = Vector2::random() * Vector2 { 50, 50 };
				randTimer = rand() % 4 + 2; // every 2-6 seconds randomly pick a new target
			}
			for (int aimTarget = 0; aimTarget < current.playerCount - 1; ++aimTarget)
				if (current.tacticoolData[aimTarget].inSight) // if in Sight and we can fire
				{
					target1 = Vector2::fromXZ(current.tacticoolData[aimTarget].lastKnownPosition);
					tankState = CHASE;
				}


			// determine the forward to the target (which is the next waypoint in the path)
			Vector2 tf = normal(target - cp);

			if (dot(cf, tf) > .87f) // if the dot product is close to lining up, move forward
				tbc.tankMove = tankNet::TankMovementOptions::FWRD;
			else // otherwise turn right until we line up!
				tbc.tankMove = tankNet::TankMovementOptions::RIGHT;
		}
	}
	 void chase()
		{
		
		
		
			if (current.currentHealth == 100)
			{
				prevhp = 0;
				prevhp = current.currentHealth - 1;
			}
		
		
			if (current.currentHealth < prevhp)
			{
				prevhp = current.currentHealth - 1;
				tankState = AVOID;
			}
			Vector2 cp = Vector2::fromXZ(current.position); // current position
			Vector2 cf = Vector2::fromXZ(current.forward);  // current forward
		
			for (int aimTarget = 0; aimTarget < current.playerCount - 1; ++aimTarget)
				if (current.tacticoolData[aimTarget].inSight) // if in Sight and we can fire
				{
					target = Vector2::fromXZ(current.tacticoolData[aimTarget].lastKnownPosition);
				}
			Vector2 tf = normal(target - cp);
		
			if (dot(cf, tf) > .87f) // if the dot product is close to lining up, move forward
				tbc.tankMove = tankNet::TankMovementOptions::FWRD;
			else // otherwise turn right until we line up!
				tbc.tankMove = tankNet::TankMovementOptions::RIGHT;
			//randTimer -= sfw::getDeltaTime();
			if (randTimer < 0)
			{
		
				randTimer = rand() % 20 + 2;
				tankState = SEEK;
			}
		}
	 void avoid()
		 {
		 
		 
		 	tbc.tankMove = tankNet::TankMovementOptions::BACK;
		 	randTimer -= sfw::getDeltaTime();
		 
		 	// If we're pretty close to the target, get a new target           
		 	if (randTimer < 0)
		 	{
		 
		 		randTimer = rand() % 2;
		 		tankState = SEEK;
		 	}
		 
		 
		 }


public:
    tankNet::TankBattleCommand update(tankNet::TankBattleStateData *state)
    {
        current = *state;
        tbc.msg = tankNet::TankBattleMessage::GAME;

        switch (turretState)
        {
        case SCAN: scan(); break;
		case AIM: aim(); break;
        case FIRE: fire(); break;
        }

        switch (tankState)
        {
        case SEEK: seek(); break;
		case CHASE: chase(); break;
		case AVOID: avoid(); break;
        }

        return tbc;
    }
};