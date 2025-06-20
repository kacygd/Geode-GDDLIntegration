#ifndef GDDLINTEGRATION_GDDLLOGINLAYER_H
#define GDDLINTEGRATION_GDDLLOGINLAYER_H

#include <Utils.h>
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include "settings/LoginSettingNodeV3.h"

using namespace geode::prelude;

class GDDLLoginLayer final : public FLAlertLayer {
    CCMenuItemSpriteExtra* m_closeBtn{};
    CCLabelBMFont* statusLabel = nullptr;
    CCTextInputNode* usernameTextField = nullptr;
    CCTextInputNode* passwordTextField = nullptr;
    CCMenuItemSpriteExtra* loginButton = nullptr;

    const inline static std::string apiEndpoint = "https://cps.ps.fhgdps.com/database/accounts/api-gddl.php";
    EventListener<web::WebTask> loginListener, meListener;
    LoginSettingNodeV3* settingNode;

    bool init() override;
    void onClose(cocos2d::CCObject* sender);
    void onLoginClicked(cocos2d::CCObject *sender);

    void prepareSearchListener();
    void prepareMeListener();
    void saveLoginData(const std::string& sid, int uid);
    void closeLoginPanel();
    void showLoadingCircle();
    void hideLoadingCircle();

    // Getting around geode::web limitations
    std::pair<std::string, std::string> getCookieValue(const char* content);

public:
    static GDDLLoginLayer* create();
    void show() override;

    void setSettingNode(LoginSettingNodeV3* settingNode);
    static int getUserIDFromUserSearchJSON(matjson::Value jsonResponse, const std::string& requestedUsername);
};

#endif //GDDLINTEGRATION_GDDLLOGINLAYER_H
