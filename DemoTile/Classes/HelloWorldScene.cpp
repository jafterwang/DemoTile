//
//  HelloWorldScene.cpp
//  TileBasedGame
//
//  Created by Clawoo on 8/17/11.
//  Copyright __MyCompanyName__ 2011. All rights reserved.
//

#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "GameOverScene.h"

//#include <string>

using namespace std;
using namespace CocosDenshion;
USING_NS_CC;

CCScene* HelloWorld::scene()
{
	// 'scene' is an autorelease object
	CCScene *scene = CCScene::create();
	
	// 'layer' is an autorelease object
	HelloWorld *layer = HelloWorld::create();

	// add layer as a child to scene
	scene->addChild(layer);

	HelloWorldHud *hud = HelloWorldHud::create();
	hud->retain();
	scene->addChild(hud);
	layer->setHud(hud);
	hud->setGameLayer(layer);
	
	// return the scene
	return scene;
}

HelloWorld::~HelloWorld()
{
	CC_SAFE_RELEASE_NULL(_tileMap);
	CC_SAFE_RELEASE_NULL(_background);
	CC_SAFE_RELEASE_NULL(_foreground);
	CC_SAFE_RELEASE_NULL(_player);
	CC_SAFE_RELEASE_NULL(_meta);
	CC_SAFE_RELEASE_NULL(_hud);
	CC_SAFE_RELEASE_NULL(_enemies);
	CC_SAFE_RELEASE_NULL(_projectiles);
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
	if ( !CCLayer::init() )
	{
		return false;
	}
	
	SimpleAudioEngine::sharedEngine()->preloadEffect("pickup.caf");
	SimpleAudioEngine::sharedEngine()->preloadEffect("hit.caf");
	SimpleAudioEngine::sharedEngine()->preloadEffect("move.caf");
	SimpleAudioEngine::sharedEngine()->playBackgroundMusic("TileMap.caf");
	
	_enemies = new CCArray;
	_projectiles = new CCArray;
	
	_tileMap = CCTMXTiledMap::create("TileMap.tmx");
    _tileMap->retain();
    
	_background = _tileMap->layerNamed("Background");
    _background->retain();
	_foreground = _tileMap->layerNamed("Foreground");
    _foreground->retain();
	
	_meta = _tileMap->layerNamed("Meta");
	_meta->retain();
	
	CCTMXObjectGroup *objects = _tileMap->objectGroupNamed("Objects");
	CCAssert(objects != NULL, "'Objects' object group not found");

	CCDictionary *spawnPoint = objects->objectNamed("SpawnPoint");
	CCAssert(spawnPoint != NULL, "SpawnPoint object not found");
	//int x = spawnPoint->objectForKey("x")->toInt();
	//int y = spawnPoint->objectForKey("y")->toInt();
    int x = spawnPoint->valueForKey("x")->intValue();
    int y = spawnPoint->valueForKey("y")->intValue();

    this->addChild(_tileMap, -1);
	
	_player = CCSprite::create("Player.png");
	_player->retain();
	
	_player->setPosition(ccp (x, y));
	
	this->addChild(_player);
	
	this->setViewpointCenter(_player->getPosition());
	
	CCArray *allObjects = objects->getObjects();
    CCObject *pObj = NULL;
    CCARRAY_FOREACH(allObjects, pObj)
	//for (it = allObjects->begin(); it != allObjects->end(); ++it)
	{
        CCDictionary *pDict = static_cast<CCDictionary*>(pObj);
		if (pDict->objectForKey(std::string("Enemy")) != NULL)
		{
			int x = pDict->valueForKey("x")->intValue();
			int y = pDict->valueForKey("y")->intValue();
			this->addEnemyAt(x, y);
		}
	}
	
    // 如果想要获得点击支持,必须设置mode
    this->setTouchMode(ccTouchesMode::kCCTouchesOneByOne);
	this->setTouchEnabled(true);
	
	_numCollected = 0;
	
	_mode = 0;
	
	this->schedule(schedule_selector(HelloWorld::testCollisions));
	
	return true;
}

void HelloWorld::setViewpointCenter(CCPoint position)
{
	CCSize winSize = CCDirector::sharedDirector()->getWinSize();
	
	int x = MAX(position.x, winSize.width / 2);
	int y = MAX(position.y, winSize.height / 2);
	
	x = MIN(x, (_tileMap->getMapSize().width * _tileMap->getTileSize().width) - winSize.width/2);
	y = MIN(y, (_tileMap->getMapSize().height * _tileMap->getTileSize().height) - winSize.height/2);
	
	CCPoint actualPosition = ccp(x, y);
	
	CCPoint centerOfView = ccp(winSize.width/2, winSize.height/2);
	
	CCPoint viewPoint = ccpSub(centerOfView, actualPosition);
	
	this->setPosition(viewPoint);
}
//void HelloWorld::registerWithTouchDispatcher()
//{
//    CCLayer::registerWithTouchDispatcher();
//    //CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
//}

bool HelloWorld::ccTouchBegan(CCTouch *touch, CCEvent *event)
{
	return true;
}

void HelloWorld::ccTouchEnded(CCTouch *touch, CCEvent *event)
{
	if (_mode == 0)
	{
	    CCPoint touchLocation = touch->getLocationInView();
	    touchLocation = CCDirector::sharedDirector()->convertToGL(touchLocation);
	    touchLocation = this->convertToNodeSpace(touchLocation);
	
	    CCPoint playerPos = _player->getPosition();
	    CCPoint diff = ccpSub(touchLocation, playerPos);
	
	    // using fabs because using abs throws a "abs in ambiguous" error
	    if (fabs(diff.x) > fabs(diff.y))
        {
            if (diff.x > 0) 
            {
                playerPos.x += _tileMap->getTileSize().width;
            }
            else
            {
                playerPos.x -= _tileMap->getTileSize().width; 
            }    
        } 
        else 
        {
            if (diff.y > 0) 
            {
                playerPos.y += _tileMap->getTileSize().height;
            }
            else
            {
                playerPos.y -= _tileMap->getTileSize().height;
            }
        }
	
	    if (playerPos.x <= (_tileMap->getMapSize().width * _tileMap->getTileSize().width) &&
            playerPos.y <= (_tileMap->getMapSize().height * _tileMap->getTileSize().height) &&
            playerPos.y >= 0 &&
            playerPos.x >= 0 ) 
        {
		    this->setPlayerPosition(playerPos);
        }
	
	    this->setViewpointCenter(_player->getPosition());
	}
	else 
    {
		// Find where the touch is
		CCPoint touchLocation = touch->getLocationInView();
		touchLocation = CCDirector::sharedDirector()->convertToGL(touchLocation);
		touchLocation = this->convertToNodeSpace(touchLocation);
		
		// Create a projectile and put it at the player's location
		CCSprite *projectile = CCSprite::create("Projectile.png");
		projectile->setPosition(_player->getPosition());
		this->addChild(projectile);
		
		// Determine where we wish to shoot the projectile to
		int realX;
		
		// Are we shooting to the left or right?
		CCPoint diff = ccpSub(touchLocation, _player->getPosition());
		if (diff.x > 0)
		{
			realX = (_tileMap->getMapSize().width * _tileMap->getTileSize().width) +
				(projectile->getContentSize().width/2);
		}
        else
        {
			realX = -(_tileMap	->getMapSize().width * _tileMap->getTileSize().width) -
				(projectile->getContentSize().width/2);
		}
		float ratio = (float) diff.y / (float) diff.x;
		int realY = ((realX - projectile->getPosition().x) * ratio) + projectile->getPosition().y;
		CCPoint realDest = ccp(realX, realY);
		
		// Determine the length of how far we're shooting
		int offRealX = realX - projectile->getPosition().x;
		int offRealY = realY - projectile->getPosition().y;
		float length = sqrtf((offRealX*offRealX) + (offRealY*offRealY));
		float velocity = 480/1; // 480pixels/1sec
		float realMoveDuration = length/velocity;
		
		// Move projectile to actual endpoint
		CCFiniteTimeAction *actionMoveTo = CCMoveTo::create(realMoveDuration, realDest);
		CCFiniteTimeAction *actionMoveDone = NULL;
        actionMoveDone = dynamic_cast<CCFiniteTimeAction*>(CCCallFuncN::create(this, callfuncN_selector(HelloWorld::projectileMoveFinished)));
		projectile->runAction(CCSequence::create(actionMoveTo, actionMoveDone, NULL));
		
		_projectiles->addObject(projectile);
	}
}

void HelloWorld::setPlayerPosition(cocos2d::CCPoint position)
{
	CCPoint tileCoord = this->tileCoordForPosition(position);
	int tileGid = _meta->tileGIDAt(tileCoord);
	if (tileGid)
	{
		CCDictionary *properties = _tileMap->propertiesForGID(tileGid);
		if (properties)
		{
			const CCString *collision = properties->valueForKey("Collidable");
			if (collision && (collision->compare("True") == 0))
			{
				SimpleAudioEngine::sharedEngine()->playEffect("hit.caf");
				return;
			}
			
			const CCString *collectable = properties->valueForKey("Collectable");
			if (collectable && (collectable->compare("True") == 0))
			{
				_meta->removeTileAt(tileCoord);
				_foreground->removeTileAt(tileCoord);
				
				_numCollected++;
				_hud->numCollectedChanged(_numCollected);
				SimpleAudioEngine::sharedEngine()->playEffect("pickup.caf");
				
				if (_numCollected == 2) {
					this->win();
				}
			}
		}
	}
	SimpleAudioEngine::sharedEngine()->playEffect("move.caf");
	_player->setPosition(position);
}

CCPoint HelloWorld::tileCoordForPosition(cocos2d::CCPoint position)
{
	int x = position.x / _tileMap->getTileSize().width;
    int y = ((_tileMap->getMapSize().height * _tileMap->getTileSize().height) - position.y) / _tileMap->getTileSize().height;
    return ccp(x, y);
}

void HelloWorld::addEnemyAt(int x, int y)
{
	CCSprite *enemy = CCSprite::create("enemy1.png");
	enemy->retain();
	enemy->setPosition(ccp(x, y));
	this->addChild(enemy);
	// Use our animation method and
	// start the enemy moving toward the player
	this->animateEnemy(enemy);
	_enemies->addObject(enemy);
}

// callback. starts another iteration of enemy movement.
void HelloWorld::enemyMoveFinished(cocos2d::CCNode *enemy)
{
	this->animateEnemy(enemy);
}

// a method to move the enemy 10 pixels toward the player
void HelloWorld::animateEnemy(cocos2d::CCNode *enemy)
{
	// speed of the enemy
	float actualDuration = 0.3;

	//rotate to face the player
	CCPoint diff = ccpSub(_player->getPosition(), enemy->getPosition());
	float angleRadians = atanf((float)diff.y / (float)diff.x);
	float angleDegrees = CC_RADIANS_TO_DEGREES(angleRadians);
	float cocosAngle = -1 * angleDegrees;
	if (diff.x < 0) {
		cocosAngle += 180;
	}
	enemy->setRotation(cocosAngle);
	
	CCPoint moveBy = ccpMult(ccpNormalize(ccpSub(_player->getPosition(),enemy->getPosition())), 10);
	
	// Create the actions
	CCFiniteTimeAction *actionMove = CCMoveBy::create(actualDuration, moveBy);
    CCFiniteTimeAction *actionMoveDone = CCCallFuncN::create(this, callfuncN_selector(HelloWorld::enemyMoveFinished));
    enemy->runAction(CCSequence::create(actionMove, actionMoveDone, NULL));
}

void HelloWorld::projectileMoveFinished(CCNode *sprite)
{
	this->removeChild(sprite, true);
	_projectiles->removeObject(sprite, true);
}

void HelloWorld::testCollisions(float dt)
{
	CCArray *projectilesToDelete = new CCArray;

	//CCMutableArray<CCSprite *>::CCMutableArrayIterator it, jt;
	// iterate through projectiles
    CCObject *pObj = NULL;
    CCARRAY_FOREACH(_projectiles, pObj)
	//for (it = _projectiles->begin(); it != _projectiles->end(); it++) 
    {
        CCSprite *projectile = static_cast<CCSprite *>(pObj);
		//CCSprite *projectile = *it;
		CCRect projectileRect = CCRectMake(projectile->getPosition().x - (projectile->getContentSize().width/2),
										   projectile->getPosition().y - (projectile->getContentSize().height/2),
										   projectile->getContentSize().width,
										   projectile->getContentSize().height);
		
		//CCMutableArray<CCSprite*> *targetsToDelete = new CCMutableArray<CCSprite*>;
        CCArray *targetsToDelete = new CCArray;		
		// iterate through enemies, see if any intersect with current projectile
        pObj = NULL;
        CCARRAY_FOREACH(_enemies, pObj)
		//for (jt = _enemies->begin(); jt != _enemies->end(); jt++)
		{
            CCSprite *target = static_cast<CCSprite *>(pObj);
			//CCSprite *target = *jt;
			CCRect targetRect = CCRectMake(
										   target->getPosition().x - (target->getContentSize().width/2), 
										   target->getPosition().y - (target->getContentSize().height/2), 
										   target->getContentSize().width, 
										   target->getContentSize().height);
			
			if (projectileRect.intersectsRect(targetRect))
            {
				targetsToDelete->addObject(target);
			}
		}
		
		// delete all hit enemies
        pObj = NULL;
        CCARRAY_FOREACH(targetsToDelete, pObj)
		//for (jt = targetsToDelete->begin(); jt != targetsToDelete->end(); jt++) 
		{
			_enemies->removeObject(pObj);
			this->removeChild(static_cast<CCNode*>(pObj), true);
		}
		
		if (targetsToDelete->count() > 0)
		{
			projectilesToDelete->addObject(projectile);
		}
		
		targetsToDelete->release();
	}
	
	// remove all the projectiles that hit.
    pObj = NULL;
    CCARRAY_FOREACH(projectilesToDelete, pObj)
	//for (it = projectilesToDelete->begin(); it != projectilesToDelete->end(); it++) 
    {
        CCSprite *projectile = static_cast<CCSprite *>(pObj);
		//CCSprite *projectile = *it;
		_projectiles->removeObject(projectile, true);
		this->removeChild(projectile, true);
	}
	
    pObj = NULL;
    CCARRAY_FOREACH(_enemies, pObj)
	//for (jt = _enemies->begin(); jt != _enemies->end(); jt++)
	{
        CCSprite *target = static_cast<CCSprite *>(pObj);
		//CCSprite *target = *jt;
		CCRect targetRect = CCRectMake(
									   target->getPosition().x - (target->getContentSize().width/2), 
									   target->getPosition().y - (target->getContentSize().height/2), 
									   target->getContentSize().width, 
									   target->getContentSize().height);
		
		if (targetRect.containsPoint( _player->getPosition())) 
        {
			this->lose();
		}
	}
}

void HelloWorld::win()
{
	GameOverScene *gameOverScene = GameOverScene::create();
	gameOverScene->getLayer()->getLabel()->setString("You Win!");
	CCDirector::sharedDirector()->replaceScene(gameOverScene);
}

void HelloWorld::lose()
{
	GameOverScene *gameOverScene = GameOverScene::create();
	gameOverScene->getLayer()->getLabel()->setString("You Lose!");
	CCDirector::sharedDirector()->replaceScene(gameOverScene);
}