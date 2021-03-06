//
//  AssetChecker.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 01/04/15.
//
//

#ifndef __BaseApp__AssetChecker__
#define __BaseApp__AssetChecker__

#include "ofMain.h"

class AssetHolder;

class AssetCheckThread : public ofThread{

public:

	void checkAssetsInThread(const vector<AssetHolder*>& assetObjects, ofMutex * mutex);

	float getProgress(){return progress;}
	ofEvent<void> eventFinishedCheckingAssets;

	int getNumObjectsToCheck(){return assetObjects.size();};
	int getNumObjectsChecked(){ return assetObjects.size() * progress; };

private:

	float progress = 0;
	ofMutex * myMutex = nullptr;
	void threadedFunction();
	vector<AssetHolder*> assetObjects;
};



class AssetChecker{

public:
	
	AssetChecker(){};

	void checkAssets(vector<AssetHolder*> assetObjects, int numThreads = std::thread::hardware_concurrency());
	void update();
	float getProgress();
	vector<float> getPerThreadProgress();

	string getDrawableState();

	//callback
	void onAssetCheckThreadFinished();

	ofEvent<void> eventFinishedCheckingAllAssets;

protected:

	bool started = false;
	int numThreadsCompleted = 0;
	vector<AssetCheckThread*> threads;
	vector<AssetHolder*> assetObjects;
	ofMutex mutex;
};

#endif /* defined(__BaseApp__AssetChecker__) */
