/****************************************************************************
 Copyright (c) 2014 cocos2d-x.org
 Copyright (c) 2015-2016 Chukong Technologies Inc.
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "Manifest.h"

#include <fstream>
#include <cstdio>
#include "cocos/base/ccMacros.h"
#include "platform/CCDevice.h"

#define KEY_VERSION             "version"
#define KEY_PACKAGE_URL         "packageUrl"
#define KEY_MANIFEST_URL        "remoteManifestUrl"
#define KEY_VERSION_URL         "remoteVersionUrl"
#define KEY_GROUP_VERSIONS      "groupVersions"
#define KEY_ENGINE_VERSION      "engineVersion"
#define KEY_UPDATING            "updating"
#define KEY_ASSETS              "assets"
#define KEY_COMPRESSED_FILES    "compressedFiles"
#define KEY_SEARCH_PATHS        "searchPaths"

#define KEY_PATH                "path"
#define KEY_MD5                 "md5"
#define KEY_GROUP               "group"
#define KEY_COMPRESSED          "compressed"
#define KEY_SIZE                "size"
#define KEY_COMPRESSED_FILE     "compressedFile"
#define KEY_DOWNLOAD_STATE      "downloadState"

NS_CC_EXT_BEGIN

static void changeHotUrl(std::string& url, const std::string& hotroot)
{
    auto index = url.find("/hotupdate");
    if (index != std::string::npos)
    {
        url.replace(0, index, hotroot);
    }
}

static int cmpVersion(const std::string& v1, const std::string& v2)
{
    int i;
    int oct_v1[4] = {0}, oct_v2[4] = {0};
    int filled1 = std::sscanf(v1.c_str(), "%d.%d.%d.%d", &oct_v1[0], &oct_v1[1], &oct_v1[2], &oct_v1[3]);
    int filled2 = std::sscanf(v2.c_str(), "%d.%d.%d.%d", &oct_v2[0], &oct_v2[1], &oct_v2[2], &oct_v2[3]);
    
    if (filled1 == 0 || filled2 == 0)
    {
        return strcmp(v1.c_str(), v2.c_str());
    }
    for (i = 0; i < 4; i++)
    {
        if (oct_v1[i] > oct_v2[i])
            return 1;
        else if (oct_v1[i] < oct_v2[i])
            return -1;
    }
    return 0;
}

Manifest::Manifest(const std::string& manifestUrl/* = ""*/)
: _versionLoaded(false)
, _loaded(false)
, _updating(false)
, _manifestRoot("")
, _remoteManifestUrl("")
, _remoteVersionUrl("")
, _version("")
, _engineVer("")
, assets(nullptr)
{
    // Init variables
    _fileUtils = FileUtils::getInstance();
    _hotupdateRoot = Device::getResString("weburl");
    CCLOG("AssetsManagerEx: getResString weburl %s", _hotupdateRoot.c_str());
    if (!_hotupdateRoot.empty() && _hotupdateRoot.back() == '/') // 去掉尾部的'/'
        _hotupdateRoot.pop_back();
    if (!manifestUrl.empty())
        parseFile(manifestUrl);
}

Manifest::Manifest(const std::string& content, const std::string& manifestRoot)
: _versionLoaded(false)
, _loaded(false)
, _updating(false)
, _manifestRoot("")
, _remoteManifestUrl("")
, _remoteVersionUrl("")
, _version("")
, _engineVer("")
{
    // Init variables
    _fileUtils = FileUtils::getInstance();
    if (content.size() > 0)
        parseJSONString(content, manifestRoot);
}

void Manifest::loadJson(const std::string& url)
{
    clear();
    std::string content;
    if (_fileUtils->isFileExist(url))
    {
        // Load file content
        content = _fileUtils->getStringFromFile(url);
        
        if (content.size() == 0)
        {
            CCLOG("Fail to retrieve local file content: %s\n", url.c_str());
        }
        else
        {
            loadJsonFromString(content);
        }
    }
}

void Manifest::loadJsonFromString(const std::string& content)
{
    if (content.size() == 0)
    {
        CCLOG("Fail to parse empty json content.");
    }
    else
    {
        // Parse file with rapid json
        try {
            _json = _json.parse(content.c_str());
            if(_json.is_object()) {
                assets = &_json[KEY_ASSETS];
            }
        }
        catch (const std::exception& e) {
            CCLOGERROR("File parse error %s", e.what());
        }
    }
}

void Manifest::parseVersion(const std::string& versionUrl)
{
    loadJson(versionUrl);
    
    if (_json.is_object())
    {
        loadVersion(_json);
    }
}

void Manifest::parseFile(const std::string& manifestUrl)
{
    loadJson(manifestUrl);
	
    if (_json.is_object())
    {
        // Register the local manifest root
        size_t found = manifestUrl.find_last_of("/\\");
        if (found != std::string::npos)
        {
            _manifestRoot = manifestUrl.substr(0, found+1);
        }
        loadManifest(_json);
    }
}

void Manifest::parseJSONString(const std::string& content, const std::string& manifestRoot)
{
    loadJsonFromString(content);
    
    if (_json.is_object())
    {
        // Register the local manifest root
        _manifestRoot = manifestRoot;
        loadManifest(_json);
    }
}

bool Manifest::isVersionLoaded() const
{
    return _versionLoaded;
}
bool Manifest::isLoaded() const
{
    return _loaded;
}

void Manifest::setUpdating(bool updating)
{
    if (_loaded && _json.is_object())
    {
        _json[KEY_UPDATING] = updating;
        _updating = updating;
    }
}

bool Manifest::versionEquals(const Manifest *b) const
{
    // Check manifest version
    if (_version != b->getVersion())
    {
        return false;
    }
    // Check group versions
    else
    {
        std::vector<std::string> bGroups = b->getGroups();
        std::unordered_map<std::string, std::string> bGroupVer = b->getGroupVerions();
        // Check group size
        if (bGroups.size() != _groups.size())
            return false;
        
        // Check groups version
        for (unsigned int i = 0; i < _groups.size(); ++i) {
            std::string gid =_groups[i];
            // Check group name
            if (gid != bGroups[i])
                return false;
            // Check group version
            if (_groupVer.at(gid) != bGroupVer.at(gid))
                return false;
        }
    }
    return true;
}

bool Manifest::versionGreaterOrEquals(const Manifest *b, const std::function<int(const std::string& versionA, const std::string& versionB)>& handle) const
{
    std::string localVersion = getVersion();
    std::string bVersion = b->getVersion();
    bool greater;
    if (handle)
    {
        greater = handle(localVersion, bVersion) >= 0;
    }
    else
    {
        greater = cmpVersion(localVersion, bVersion) >= 0;
    }
    return greater;
}

bool Manifest::versionGreater(const Manifest *b, const std::function<int(const std::string& versionA, const std::string& versionB)>& handle) const
{
    std::string localVersion = getVersion();
    std::string bVersion = b->getVersion();
    bool greater;
    if (handle)
    {
        greater = handle(localVersion, bVersion) > 0;
    }
    else
    {
        greater = cmpVersion(localVersion, bVersion) > 0;
    }
    return greater;
}

std::unordered_map<std::string, Manifest::AssetDiff> Manifest::genDiff(const Manifest *b) const
{
    std::unordered_map<std::string, AssetDiff> diff_map;
    const std::unordered_map<std::string, Asset> &bAssets = b->getAssets();
    
    std::string key;
    Asset valueA;
    Asset valueB;
    
    std::unordered_map<std::string, Asset>::const_iterator valueIt, it;
    for (it = _assets.begin(); it != _assets.end(); ++it)
    {
        key = it->first;
        valueA = it->second;
        
        // Deleted
        valueIt = bAssets.find(key);
        if (valueIt == bAssets.cend()) {
            AssetDiff diff;
            diff.asset = valueA;
            diff.type = DiffType::DELETED;
            diff_map.emplace(key, diff);
            continue;
        }
        
        // Modified
        valueB = valueIt->second;
        if (valueA.md5 != valueB.md5) {
            AssetDiff diff;
            diff.asset = valueB;
            diff.type = DiffType::MODIFIED;
            diff_map.emplace(key, diff);
        }
    }
    
    for (it = bAssets.begin(); it != bAssets.end(); ++it)
    {
        key = it->first;
        valueB = it->second;
        
        // Added
        valueIt = _assets.find(key);
        if (valueIt == _assets.cend()) {
            AssetDiff diff;
            diff.asset = valueB;
            diff.type = DiffType::ADDED;
            diff_map.emplace(key, diff);
        }
    }
    
    return diff_map;
}

void Manifest::genResumeAssetsList(DownloadUnits *units) const
{
    for (auto it = _assets.begin(); it != _assets.end(); ++it)
    {
        Asset asset = it->second;
        
        if (asset.downloadState != DownloadState::SUCCESSED && asset.downloadState != DownloadState::UNMARKED)
        {
            DownloadUnit unit;
            unit.customId = it->first;
            unit.srcUrl = _packageUrl + asset.path;
            unit.storagePath = _manifestRoot + asset.path;
            unit.size = asset.size;
            units->emplace(unit.customId, unit);
        }
    }
}

std::vector<std::string> Manifest::getSearchPaths() const
{
    std::vector<std::string> searchPaths;
    searchPaths.push_back(_manifestRoot);
    
    for (int i = (int)_searchPaths.size()-1; i >= 0; i--)
    {
        std::string path = _searchPaths[i];
        if (path.size() > 0 && path[path.size() - 1] != '/')
            path.append("/");
        path = _manifestRoot + path;
        searchPaths.push_back(path);
    }
    return searchPaths;
}

void Manifest::prependSearchPaths()
{
    std::vector<std::string> searchPaths = FileUtils::getInstance()->getSearchPaths();
    std::vector<std::string>::iterator iter = searchPaths.begin();
    bool needChangeSearchPaths = false;
    if (std::find(searchPaths.begin(), searchPaths.end(), _manifestRoot) == searchPaths.end())
    {
        searchPaths.insert(iter, _manifestRoot);
        needChangeSearchPaths = true;
    }
    
    for (int i = (int)_searchPaths.size()-1; i >= 0; i--)
    {
        std::string path = _searchPaths[i];
        if (path.size() > 0 && path[path.size() - 1] != '/')
            path.append("/");
        path = _manifestRoot + path;
        iter = searchPaths.begin();
        searchPaths.insert(iter, path);
        needChangeSearchPaths = true;
    }
    if (needChangeSearchPaths)
    {
        FileUtils::getInstance()->setSearchPaths(searchPaths);
    }
}


const std::string& Manifest::getPackageUrl() const
{
    return _packageUrl;
}

const std::string& Manifest::getManifestFileUrl() const
{
    return _remoteManifestUrl;
}

const std::string& Manifest::getVersionFileUrl() const
{
    return _remoteVersionUrl;
}

const std::string& Manifest::getVersion() const
{
    return _version;
}

const std::vector<std::string>& Manifest::getGroups() const
{
    return _groups;
}

const std::unordered_map<std::string, std::string>& Manifest::getGroupVerions() const
{
    return _groupVer;
}

const std::string& Manifest::getGroupVersion(const std::string &group) const
{
    return _groupVer.at(group);
}

const std::unordered_map<std::string, Manifest::Asset>& Manifest::getAssets() const
{
    return _assets;
}

void Manifest::setAssetDownloadState(const std::string &key, const Manifest::DownloadState &state)
{
    auto valueIt = _assets.find(key);
    if (valueIt != _assets.end())
    {
        valueIt->second.downloadState = state;
        // Update json object
        if(assets != nullptr)
        {
            auto asset = assets->find(key);
            if (asset != assets->end())
            {
                (*asset)[KEY_DOWNLOAD_STATE] = (int)state;
            }
        }
    }
}

void Manifest::clear()
{
    if (_versionLoaded || _loaded)
    {
        _groups.clear();
        _groupVer.clear();
        
        _remoteManifestUrl = "";
        _remoteVersionUrl = "";
        _version = "";
        _engineVer = "";
        
        _versionLoaded = false;
    }
    
    if (_loaded)
    {
        _assets.clear();
        _searchPaths.clear();
        _loaded = false;
    }
}

Manifest::Asset Manifest::parseAsset(const std::string &path, const nlohmann::json &json) {
    Asset asset;
    asset.path = path;
    tryGetTo(json, KEY_MD5, asset.md5);
    tryGetTo(json, KEY_PATH, asset.path);
    tryGetTo(json, KEY_COMPRESSED, asset.compressed);
    tryGetTo(json, KEY_SIZE, asset.size);
    tryGetTo(json, KEY_DOWNLOAD_STATE, asset.downloadState);
    return asset;
}

void Manifest::loadVersion(const nlohmann::json &json) {
    // Retrieve remote manifest url
    tryGetTo(json, KEY_MANIFEST_URL, _remoteManifestUrl);
    changeHotUrl(_remoteManifestUrl, _hotupdateRoot); // 修改热更新地址
    // Retrieve remote version url
    tryGetTo(json, KEY_VERSION_URL, _remoteVersionUrl);
    changeHotUrl(_remoteVersionUrl, _hotupdateRoot); // 修改热更新地址
    // Retrieve local version
    tryGetTo(json, KEY_VERSION, _version);

    // Retrieve local group version
    {
        auto it = json.find(KEY_GROUP_VERSIONS);
        if (it !=json.end())
        {
            auto groupVers = it.value();
            {
                for (auto itr = groupVers.begin(); itr != groupVers.end(); ++itr) {
                    _groups.push_back(itr.key());
                    _groupVer.emplace(itr.key(), itr.value().get<std::string>());
                }
            }
        }
    }

    // Retrieve local engine version
    tryGetTo(json, KEY_ENGINE_VERSION, _engineVer);

    // Retrieve updating flag
    tryGetTo(json, KEY_UPDATING, _updating);

    _versionLoaded = true;
}

void Manifest::loadManifest(const nlohmann::json &json) {
    loadVersion(json);

    // Retrieve package url
    tryGetTo(json, KEY_PACKAGE_URL, _packageUrl);
    changeHotUrl(_packageUrl, _hotupdateRoot); // 修改热更新地址

    // Retrieve all assets
    auto iterator = json.find(KEY_ASSETS);
    if (iterator != json.end()) {
        const nlohmann::json &assets = iterator.value();
        if (assets.is_object()) {
            CostTime time("_assets.emplace");
            for (auto itr = assets.begin(); itr != assets.end(); ++itr) {
                _assets.emplace(itr.key(), parseAsset(itr.key(), *itr));
            }
        }
    }

    // Retrieve all search paths
    if (json.find(KEY_SEARCH_PATHS) != json.end()) {
        const nlohmann::json &paths = json[KEY_SEARCH_PATHS];
        if (paths.is_array()) {
            for (const auto& path : paths) {
                if (path.is_string()) {
                    _searchPaths.push_back(path.get<std::string>());
                }
            }
        }
    }

    _loaded = true;
}

void Manifest::saveToFile(const std::string &filepath)
{
    std::ofstream output(FileUtils::getInstance()->getSuitableFOpen(filepath), std::ofstream::out);
    if(!output.bad())
        output << _json.dump() << std::endl;
}

NS_CC_EXT_END
