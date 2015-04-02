//
//  AssetHolder.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 27/03/15.
//
//

#include "AssetHolder.h"
#include "ofxChecksum.h"
#include "ofxThreadSafeLog.h"

AssetHolder::AssetDescriptor AssetHolder::emptyAsset = AssetHolder::AssetDescriptor();
int AssetHolder::minimumFileSize = 1024;


AssetHolder::AssetHolder(){
	isSetup = false;
	isDownloadingData = false;
}


void AssetHolder::setup(const string& directoryForAssets_, AssetUsagePolicy assetOkPolicy_,
						AssetDownloadPolicy downloadPolicy_){
	isSetup = true;
	directoryForAssets = ofFilePath::addTrailingSlash(directoryForAssets_);
	assetOkPolicy = assetOkPolicy_;
	downloadPolicy = downloadPolicy_;
}


string AssetHolder::addRemoteAsset(const string& url, const string& sha1, AssetSpecs spec){

	ASSET_HOLDER_SETUP_CHECK;

	AssetDescriptor ad;
	ad.fileName = ofFilePath::getFileName(url);
	ad.absolutePath = ofToDataPath(directoryForAssets + "/" + ad.fileName, true);

	map<string, AssetDescriptor>::iterator it = assets.find(ad.absolutePath);
	if(it == assets.end()){ //we dont have this one
		ad.location = REMOTE;
		ad.extension = ofFilePath::getFileExt(url);
		ad.specs = spec;
		ad.type = typeFromExtension(ad.extension);
		ad.url = url;
		ad.sha1 = sha1;
		assets[ad.absolutePath] = ad;
	}else{
		ofLogError("AssetHolder") << " cant add this remote asset, already have it! " << ad.absolutePath;
	}
};


string AssetHolder::addLocalAsset(const string& localPath, AssetSpecs spec){

	ASSET_HOLDER_SETUP_CHECK;

	AssetDescriptor ad;
	ad.absolutePath = ofToDataPath(localPath, true);

	map<string, AssetDescriptor>::iterator it = assets.find(ad.absolutePath);
	if(it == assets.end()){ //we dont have this one
		ad.extension = ofFilePath::getFileExt(localPath);
		ad.type = typeFromExtension(ad.extension);
		ad.specs = spec;
		ad.fileName = ofFilePath::getFileName(localPath);
		assets[ad.absolutePath] = ad;
	}else{
		ofLogError("AssetHolder") << " cant add this remote asset, already have it! " << ad.absolutePath;
	}
}

bool AssetHolder::areAllAssetsOK(){
	int numOK = 0;
	map<string, AssetDescriptor>::iterator it = assets.begin();
	while( it != assets.end()){

		if (isReadyToUse(it->second)){
			numOK++;
		}
		++it;
	}
	return numOK == assets.size();
}

void AssetHolder::downloadsFinished(ofxBatchDownloaderReport & report){

	int n = report.responses.size();
	for(int i = 0; i < n; i++){

		ofxSimpleHttpResponse & r = report.responses[i];

		if (remoteAssetExistsInDB(r.url)){

			AssetDescriptor & d = getAssetDescForURL(r.url);
			d.status.downloaded = true;
			d.status.downloadOK = r.ok;
			if (r.expectedChecksum == d.sha1 && r.checksumOK){
				d.status.sha1Match = true;
			}else{
				d.status.sha1Match = false;
			}
		}else{
			ofLogError("AssetHolder") << "asset downloaded but I dont know about it !?";
		}
	}
}


void AssetHolder::updateLocalAssetsStatus(){

	map<string, AssetDescriptor>::iterator it = assets.begin();
	while( it != assets.end()){
		checkLocalAssetStatus(it->second);
		++it;
	}
}


void AssetHolder::checkLocalAssetStatus(AssetDescriptor & d){

	if(d.absolutePath.size() == 0){
		ofLogError("AssetHolder") << "Asset with no 'absolutePath'; cant checkLocalAssetStatus!";
		return;
	}
	ofFile f;
	f.open( d.absolutePath );

	d.status.checked = true;

	if(f.exists()){

		d.status.localFileExists = true;

		if (d.hasSha1()){
			d.status.sha1Supplied = true;
			d.status.localFileSha1Checked = true;
			d.status.sha1Match = ofxChecksum::sha1(d.absolutePath, d.sha1, false/*verbose*/);

			if (d.status.sha1Match){
				ofxThreadSafeLog::one()->append(assetLogFile, "'" + string(d.absolutePath) + "' EXISTS and SHA1 OK 😄");
			}else{
				if (f.getSize() < minimumFileSize){
					d.status.fileTooSmall = true;
					ofxThreadSafeLog::one()->append(assetLogFile, "'" + string(d.absolutePath) + "' file is empty!! 😨");
				}else{
					ofxThreadSafeLog::one()->append(assetLogFile, "'" + string(d.absolutePath) + "' CORRUPT! (sha1 missmatch) 💩");
				}
			}
		}else{ //no sha1 supplied!
			d.status.sha1Supplied = false;
			ofxThreadSafeLog::one()->append(assetLogFile, "'" + string(d.absolutePath) + "' (sha1 not supplied) 🌚");
		}

	}else{
		d.status.localFileExists = false;
		ofxThreadSafeLog::one()->append(assetLogFile, "'" + string(d.absolutePath) + "' Does NOT EXIST! 😞");
	}
	f.close();
}



vector<string> AssetHolder::downloadMissingAssets(ofxDownloadCentral& downloader){

	if(!isDownloadingData){

		vector<string> urls;
		vector<string> sha1s;

		map<string, AssetDescriptor>::iterator it = assets.begin();

		while( it != assets.end() ){

			AssetDescriptor & d = it->second;

			if(shouldDownload(d)){
				urls.push_back(d.url);
				sha1s.push_back(d.sha1);
			}
			++it;
		}

		if(urls.size()){
			downloader.downloadResources(urls,								//list of urls
										 sha1s,								//matching list of sha1s TODO!
										 this,								//who will get notified
										 &AssetHolder::downloadsFinished,	//callback
										 directoryForAssets					//where to download the assets
										 );
			isDownloadingData = true;
		}
		return urls;
	}else{
		ofLogError("AssetHolder") << "Cant download now! Already downloading data...";
	}
	return vector<string>();
}
