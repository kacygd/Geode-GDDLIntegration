#include "GDDLLoginLayer.h"

#include "Utils.h"
#include "GDDLRatingSubmissionLayer.h"
#include <Geode/Loader.hpp>
#include <Geode/ui/LoadingSpinner.hpp>

bool GDDLLoginLayer::init() {
    if (!FLAlertLayer::init(75)) return false;

    const CCPoint popupSize = {280.0f, 185.0f};
    const auto winSize = CCDirector::sharedDirector()->getWinSize();

    // background
    const auto bg = CCScale9Sprite::create(Utils::getGrayPopupBG().c_str(), {0.0f, 0.0f, 80.0f, 80.0f});
    bg->setContentSize(popupSize);
    bg->setPosition({winSize.width / 2, winSize.height / 2});
    bg->setID("gddl-login-bg"_spr);
    m_mainLayer->addChild(bg, -1);

    // menu with the main layout
    m_buttonMenu = cocos2d::CCMenu::create();
    m_buttonMenu->setID("gddl-login-button-menu"_spr);
    m_buttonMenu->setContentSize(popupSize);
    m_buttonMenu->setPosition({winSize.width / 2 - popupSize.x / 2, winSize.height / 2 - popupSize.y / 2});
    m_mainLayer->addChild(m_buttonMenu, 10);
    // title
    const auto title = CCLabelBMFont::create("GDPS Login", "goldFont.fnt");
    title->setID("gddl-login-title"_spr);
    title->setPosition({popupSize.x / 2, popupSize.y - 20.0f});
    m_buttonMenu->addChild(title);
    // close button
    const auto closeButtonSprite = Utils::getGrayPopupCloseButton();
    m_closeBtn = CCMenuItemSpriteExtra::create(closeButtonSprite, this, menu_selector(GDDLLoginLayer::onClose));
    m_buttonMenu->addChild(m_closeBtn);
    m_closeBtn->setPosition({3.0f, popupSize.y - 3.0f});

    // content
    // status text
    statusLabel = CCLabelBMFont::create("", "chatFont.fnt");
    statusLabel->setID("gddl-login-status-label"_spr);
    statusLabel->setPosition({popupSize.x / 2, popupSize.y - 38.0f});
    statusLabel->setScale(0.7f);
    m_buttonMenu->addChild(statusLabel);
    // username text + textfield
    const auto usernameLabel = CCLabelBMFont::create("Username", "bigFont.fnt");
    usernameLabel->setScale(0.5f);
    usernameLabel->setID("gddl-login-username-label"_spr);
    usernameLabel->setPosition({popupSize.x / 2 - 60.0f, popupSize.y - 50.0f});
    m_buttonMenu->addChild(usernameLabel);
    Utils::createTextInputNode(m_buttonMenu, usernameTextField, "bigFont.fnt", "", 64, {200.0f, 25.0f},
                               {popupSize.x / 2, popupSize.y - 75.0f});
    usernameTextField->setAllowedChars(Utils::hopefullyAllCharactersAnyoneWillEverNeed);
    usernameTextField->setString(Mod::get()->getSavedValue<std::string>("login-username", ""));
    // password text + textfield
    const auto passwordLabel = CCLabelBMFont::create("Password", "bigFont.fnt");
    passwordLabel->setScale(0.5f);
    passwordLabel->setID("gddl-login-password-label"_spr);
    passwordLabel->setPosition({popupSize.x / 2 - 60.0f, popupSize.y - 100.0f});
    m_buttonMenu->addChild(passwordLabel);
    Utils::createTextInputNode(m_buttonMenu, passwordTextField, "bigFont.fnt", "", 64, {200.0f, 25.0f},
                               {popupSize.x / 2, popupSize.y - 125.0f});
    passwordTextField->setAllowedChars(Utils::hopefullyAllCharactersAnyoneWillEverNeed);
    passwordTextField->m_usePasswordChar = true;
    // login button
    const auto loginButtonSprite = ButtonSprite::create("Log in", "bigFont.fnt", "GJ_button_02.png");
    loginButtonSprite->setScale(0.6f);
    loginButton = CCMenuItemSpriteExtra::create(loginButtonSprite, this,
                                               menu_selector(GDDLLoginLayer::onLoginClicked));
    loginButton->setID("gddl-login-login-button"_spr);
    loginButton->setPosition({popupSize.x / 2, popupSize.y - 160.0f});
    m_buttonMenu->addChild(loginButton);
    m_buttonMenu->reorderChild(loginButton, 10);

    prepareSearchListener();

    return true;
}

void GDDLLoginLayer::onClose(cocos2d::CCObject *sender) {
    setKeypadEnabled(false);
    removeFromParentAndCleanup(true);
}

void GDDLLoginLayer::onLoginClicked(cocos2d::CCObject *sender) {
    std::string username = std::string(usernameTextField->getString());
    if (username.empty()) {
        Notification::create("Missing username", NotificationIcon::Warning, 2)->show();
        return;
    }
    std::string password = std::string(passwordTextField->getString());
    if (password.empty()) {
        Notification::create("Missing password", NotificationIcon::Warning, 2)->show();
        return;
    }

    // Prepare POST request to GDPS API
    auto req = web::WebRequest();
    req.header("User-Agent", Utils::getUserAgent());
    req.form({
        {"userName", username},
        {"password", password}
    });
    showLoadingCircle();
    loginListener.setFilter(req.post("https://cps.ps.fhgdps.com/database/accounts/loginGJAccount.php"));
}

void GDDLLoginLayer::prepareSearchListener() {
    loginListener.bind([this](web::WebTask::Event *e) {
        if (web::WebResponse *res = e->getValue()) {
            std::string response = res->string().unwrapOr("");
            hideLoadingCircle();

            // Parse response (accountID,userID or error code)
            if (res->code() == 200 && !response.empty() && response != "-1" && response != "-12") {
                // Successful response, e.g., "123,456"
                size_t commaPos = response.find(',');
                if (commaPos != std::string::npos) {
                    std::string accountIDStr = response.substr(0, commaPos);
                    std::string userIDStr = response.substr(commaPos + 1);
                    try {
                        int accountID = std::stoi(accountIDStr);
                        int userID = std::stoi(userIDStr);

                        // Save login data
                        saveLoginData(accountID, userID);
                        Notification::create("Logged in!", NotificationIcon::Success, 2)->show();
                        RatingsManager::clearSubmissionCache();
                        closeLoginPanel();
                    } catch (...) {
                        Notification::create("Invalid response format", NotificationIcon::Error, 2)->show();
                    }
                } else {
                    Notification::create("Invalid response format", NotificationIcon::Error, 2)->show();
                }
            } else {
                // Handle errors
                std::string error;
                if (response == "-1") {
                    error = "Invalid username or password";
                } else if (response == "-12") {
                    error = "Account is banned or inactive";
                } else {
                    error = "Unknown error";
                }
                Notification::create(error, NotificationIcon::Error, 2)->show();
            }
        } else if (e->isCancelled()) {
            hideLoadingCircle();
            Notification::create("Request cancelled", NotificationIcon::Error, 2)->show();
        }
    });
}

void GDDLLoginLayer::saveLoginData(int accountID, int userID) {
    Mod::get()->setSavedValue("login-username", std::string(usernameTextField->getString()));
    Mod::get()->setSavedValue("login-accountid", accountID);
    Mod::get()->setSavedValue("login-userid", userID);
}

void GDDLLoginLayer::closeLoginPanel() {
    if (settingNode != nullptr) {
        settingNode->updateFromOutside();
    }
    onClose(nullptr);
}

void GDDLLoginLayer::showLoadingCircle() {
    const auto loadingSpinner = LoadingSpinner::create(15.0f);
    loadingSpinner->setAnchorPoint({0.0f, 0.5f});
    loadingSpinner->setPosition({loginButton->getPositionX() + loginButton->getScaledContentWidth() / 2 + 5.0f, loginButton->getPositionY()});
    loadingSpinner->setID("gddl-login-loading-spinner"_spr);
    m_buttonMenu->addChild(loadingSpinner);
}

void GDDLLoginLayer::hideLoadingCircle() {
    m_buttonMenu->removeChildByID("gddl-login-loading-spinner"_spr);
}

size_t GDDLLoginLayer::writeCallback(char *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::pair<std::string, std::string> GDDLLoginLayer::getCookieValue(const char *content) {
    std::string cookie = content;
    int equalsPos = cookie.find('=');
    std::string cookieName = cookie.substr(0, equalsPos);
    std::string cookieValue = cookie.substr(equalsPos + 1, cookie.find(';', equalsPos) - equalsPos - 1);
    return {cookieName, cookieValue};
}

GDDLLoginLayer *GDDLLoginLayer::create() {
    if (const auto newLayer = new GDDLLoginLayer(); newLayer != nullptr && newLayer->init()) {
        newLayer->autorelease();
        return newLayer;
    } else {
        delete newLayer;
        return nullptr;
    }
}

void GDDLLoginLayer::show() {
    FLAlertLayer::show();
    cocos::handleTouchPriority(this);
}

void GDDLLoginLayer::setSettingNode(LoginSettingNodeV3 *settingNode) {
    this->settingNode = settingNode;
}

int GDDLLoginLayer::getUserIDFromUserSearchJSON(matjson::Value jsonResponse, const std::string& requestedUsername) {
    if (!jsonResponse.isArray()) {
        return -2;
    }
    const auto resultsList = jsonResponse.asArray().unwrap();
    int id = -1;
    for (const auto& result : resultsList) {
        if (!result.contains("Name") || !result.contains("ID")) {
            continue;
        }
        if (result["Name"] == requestedUsername) {
            id = result["ID"].asInt().unwrap();
            break;
        }
    }
    return id;
}
