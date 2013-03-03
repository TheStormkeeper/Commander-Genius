#ifndef CBIPSHIP_H
#define CBIPSHIP_H

#include "engine/galaxy/common/ai/CStunnable.h"

namespace galaxy
{

class CBipShip : public CStunnable
{
  
public:
  CBipShip(CMap *pmap, const Uint16 foeID, const Uint32 x, const Uint32 y);
  
  /**
    * When it is moving normally
    */
  void processMoving();

  void processTheShot();
  
  void processTurning();
  
  void processHitting();
  
  void processCrashing();
  
  void processWreckage();
  
  void processSmoke();  
  
  bool isNearby(CSpriteObject &theObject);

  void process();

  /**
    * What happens if the slug gets touched by another object
    */
  void getTouchedBy(CSpriteObject &theObject);

private:
  int mTimer;
    
};

};

#endif // CBIPSHIP_H
