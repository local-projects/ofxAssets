//
//  AssetHolderUtils.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 27/03/15.
//
//

#include "AssetHolder.h"

bool AssetHolder::localAssetExistsInDB(const string& relativePath){
	auto it = assets.find(relativePath);
	return it != assets.end();
}


bool AssetHolder::remoteAssetExistsInDB(const string& url){

	auto it = assets.begin();
	while( it != assets.end()){
		if(it->second.url == url){
			return true;
		}
		++it;
	}
	return false;
}


ofxAssets::Descriptor&
AssetHolder::getAssetDescForPath(const string& relativePath){ //relative to data
	auto it = assets.find(relativePath);
	if(it != assets.end()){
		return it->second;
	}
	return emptyAsset;
}


ofxAssets::Descriptor&
AssetHolder::getAssetDescForURL(const string& url){
	auto it = assets.begin();
	while( it != assets.end()){
		if(it->second.url == url){
			return it->second;
		}
		++it;
	}
	return emptyAsset;
}


vector<ofxAssets::Descriptor>
AssetHolder::getAssetDescriptorsForType(ofxAssets::Type type){

	vector<ofxAssets::Descriptor> retAssets;
	auto it = assets.begin();
	while( it != assets.end()){
		if(it->second.type == type){
			retAssets.push_back(it->second);
		}
		++it;
	}
	return retAssets;
}


void AssetHolder::addTagsforAsset(const string & relPath, vector<string> tags){

	auto it = assets.find(relPath);
	if(it != assets.end()){
		for(auto & tag : tags){
			string objectID = it->second.relativePath; //assets inside an AssetHodler are indexed by they relative path
			this->tags.addTagForObject(objectID, Tag<TagCategory>(tag, CATEGORY));
		}
	}
}


vector<ofxAssets::Descriptor>
AssetHolder::getAssetDescsWithTag(const string & tag){

	vector<ofxAssets::Descriptor> ads;
	vector<string> paths = tags.getObjectsWithTag(Tag<TagCategory>(tag, CATEGORY));
	for(auto & path : paths){
		auto it = assets.find(path);
		if(it != assets.end()){
			ads.push_back(it->second);
		}
	}
	return ads;
}

ofxAssets::UserInfo&
AssetHolder::getUserInfoForPath(const string& relpath){

	auto it = assets.find(relpath);
	if(it != assets.end()){
		return it->second.userInfo;
	}
	ofLogError("AssetHolder") << "getUserInfoForPath() no such asset! '" << relpath << "'";
	return emptyUserInfo;
}


string AssetHolder::toString(ofxAssets::Stats &s){

	string msg = "NumAssets: " + ofToString(s.numAssets) +
	" NumMissingLocalFile: " + ofToString(s.numMissingFile) +
	" numSha1Missmatch: " + ofToString(s.numSha1Missmatch) +
	" NumFileTooSmall: " + ofToString(s.numFileTooSmall) +
	" NumOK: " + ofToString(s.numOK) +
	" NumDownloadFailed: " + ofToString(s.numDownloadFailed) +
	" NumNoSha1Supplied: " + ofToString(s.numNoSha1Supplied);
	return msg;
}


ofxAssets::Descriptor&
AssetHolder::getAssetDescAtIndex(int i){

	if(i >= 0 && i < assetAddOrder.size()){
		return assets[assetAddOrder[i]];
	}
	return  emptyAsset;
}


int AssetHolder::getNumAssets(){
	return assets.size();
}

ofxAssets::Type
AssetHolder::typeFromExtension(const string& extension){

	ofxAssets::Type type = ofxAssets::TYPE_UNKNOWN;

	string lce = ofToLower(extension);
	if(lce == "jpg" || lce == "jpeg" || lce == "png" || lce == "gif" || lce == "tiff" ||
	   lce == "tga" || lce == "bmp"){
		type = ofxAssets::IMAGE;
	}
	else if(lce == "mov" || lce == "mp4" || lce == "avi" || lce == "mpg" || lce == "mpeg" ||
			lce == "mkv" || lce == "vob" || lce == "qt" || lce == "wmv" || lce == "m4v" ||
			lce == "mp2" || lce == "m2v" || lce == "m4v"){
		type = ofxAssets::VIDEO;
	}
	else if (lce == "aif" || lce == "aiff" || lce == "mp3" || lce == "wav" || lce == "mp4" ||
			 lce == "aac" || lce == "flac" || lce == "m4a" || lce == "wma"){
		type = ofxAssets::AUDIO;
	}
	else if (lce == "json"){
		type = ofxAssets::JSON;
	}
	else if (lce == "txt" || lce == "log"){
		type = ofxAssets::TEXT;
	}
	else if (lce == "ttf" || lce == "otf"){
		type = ofxAssets::FONT;
	}
	ofLogError("AssetHolder") << "Can't recognize Asset Type from file extension: \"" << extension << "\"";
	return type;
}


bool AssetHolder::shouldDownload(const ofxAssets::Descriptor &d){

	bool shouldDownload = false;
	//lets see if we should download this asset
	if(d.status.checked){
		if(assetOkPolicy.fileMissing && !d.status.localFileExists) return true;
		if(assetOkPolicy.fileExistsAndNoSha1Provided && !d.status.sha1Supplied) return true;
		if(assetOkPolicy.fileExistsAndProvidedSha1Missmatch && !d.status.sha1Match) return true;
		if(assetOkPolicy.fileExistsAndProvidedSha1Match && !d.status.sha1Match) return true;
		if(!d.status.sha1Match){ //if we have a sha1 match - file size is irrelevant so no more tests to run
			if(assetOkPolicy.fileTooSmall && d.status.fileTooSmall) return true;
		}

	}else{
		ofLogError("AssetHolder") << "cant decide wether to download or not - havent checked for local files yet!";
	}
	return shouldDownload;
}

bool AssetHolder::isReadyToUse(const ofxAssets::Descriptor &d){

	bool isOKtoUse = false;

	//lets see if we should use this asset
	if(d.status.checked){
		bool useIf_exists = (d.status.localFileExists || assetOkPolicy.fileMissing);
		bool useIf_sha1_exists = (assetOkPolicy.fileExistsAndNoSha1Provided || d.status.sha1Supplied);

		bool useIf_sha1;
		if(assetOkPolicy.fileExistsAndProvidedSha1Match){
			useIf_sha1 = d.status.sha1Match;
		}else{
			useIf_sha1 = d.status.sha1Match || assetOkPolicy.fileExistsAndProvidedSha1Missmatch;
		}
		bool useIf_tooSmall;
		if(!d.status.fileTooSmall){
			useIf_tooSmall = true;
		}else{ //too small!
			useIf_tooSmall = assetOkPolicy.fileTooSmall;
		}

		isOKtoUse = useIf_tooSmall && useIf_sha1 && useIf_sha1_exists && useIf_exists;
	}else{
		ofLogError("AssetHolder") << "cant decide wether to USE or not - havent checked for local files yet!";
	}
	return isOKtoUse;
}
