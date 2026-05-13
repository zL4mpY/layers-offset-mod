#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

bool vecContains(std::vector<int> vec, int val) {
    for (int i : vec) {
        if (i == val) return true;
    }

    return false;
}

bool showWarning = true;

$on_mod(Loaded) {
    showWarning = Mod::get()->getSettingValue<bool>("show-warning");

    listenForSettingChanges<bool>("show-warning", [](bool value) {
        showWarning = value;
    });
}

class ReoffsetLayersPopup : public geode::Popup {
protected:
    bool init() {
        if (!Popup::init(240.f, 160.f))
            return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto center = winSize / 2.f;

        auto const center2 = CCSize(240, 160) / 2;

        this->setTitle("Reoffset layers");

        this->layersField = geode::TextInput::create(100.f, "0, 1, 2-4, 5-8", "chatFont.fnt");
        this->layersField->setFilter("0123456789-, ");
        this->layersField->setLabel("Layer to reoffset (number, range):");
        this->layersField->setPosition(center2 + ccp(0, 25));

        this->reoffsetCountField = geode::TextInput::create(100.f, "1", "chatFont.fnt");
        this->reoffsetCountField->setCommonFilter(CommonFilter::Int);
        this->reoffsetCountField->setLabel("Reoffset by:");
        this->reoffsetCountField->setPosition(center2 + ccp(0, -20));

        this->editorLayer2Toggle = CCMenuItemToggler::createWithStandardSprites(this, nullptr, 0.5f);
        this->editorLayer2Toggle->setPosition(center2 + ccp(85.f, 20.f));

        auto editorLayer2ToggleLabel = CCLabelBMFont::create("Editor layer 2", "goldFont.fnt");
        editorLayer2ToggleLabel->setScale(0.2f);
        editorLayer2ToggleLabel->setPosition(center2 + ccp(85.f, 35.f));

        auto infoSprite = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        infoSprite->setScale(0.3f);

        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoSprite,
            this,
            menu_selector(ReoffsetLayersPopup::onInfo)
        );
        infoBtn->setTag(1);
        infoBtn->setPosition(center2 + ccp(110.f, 35.f));

        this->onlySelectedToggle = CCMenuItemToggler::createWithStandardSprites(this, nullptr, 0.5f);
        this->onlySelectedToggle->setPosition(center2 + ccp(85.f, -20.f));

        auto onlySelectedToggleLabel = CCLabelBMFont::create("Only selected\nobjects", "goldFont.fnt");
        onlySelectedToggleLabel->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
        onlySelectedToggleLabel->setScale(0.2f);
        onlySelectedToggleLabel->setPosition(center2 + ccp(85.f, -2.5f));

        auto infoBtn2 = CCMenuItemSpriteExtra::create(
            infoSprite,
            this,
            menu_selector(ReoffsetLayersPopup::onInfo)
        );
        infoBtn2->setTag(2);
        infoBtn2->setPosition(center2 + ccp(110.f, -2.5f));

        auto confirmButtonSprite = ButtonSprite::create("Confirm");
        auto confirmButton = CCMenuItemSpriteExtra::create(confirmButtonSprite, this, menu_selector(ReoffsetLayersPopup::onConfirm));
        confirmButton->setPosition(center2 + ccp(0, -55));
        
        m_mainLayer->addChild(this->layersField);
        m_mainLayer->addChild(this->reoffsetCountField);
        m_mainLayer->addChild(editorLayer2ToggleLabel);
        m_mainLayer->addChild(onlySelectedToggleLabel);
        
        m_buttonMenu->addChild(this->onlySelectedToggle);
        m_buttonMenu->addChild(this->editorLayer2Toggle);
        m_buttonMenu->addChild(infoBtn);
        m_buttonMenu->addChild(infoBtn2);
        m_buttonMenu->addChild(confirmButton);

        return true;
    }

    void onInfo(CCObject* sender) {
        std::string message;

        switch (sender->getTag()) {
            case 1: message = "Make changes <cy>only</c> to the editor layer 2 property."; break;
            case 2: message = "Affect <cy>only</c> selected objects."; break;
        }

        FLAlertLayer::create(
            "Info",
            message,
            "OK"
        )->show();
    }

    void onConfirm(CCObject* sender) {
        if (showWarning) {
            geode::createQuickPopup(
                "Warning", 
                "This action <cr>cannot</c> be undone. Do you <cy>really</c> want to proceed?\n"\
                "(you can disable the confirmation popup in the mod settings if you want)",
                "No", "Yes",
                [this, sender](auto, bool btn2) {
                    if (btn2) {
                        if (perform()) this->onClose(sender);
                    }
                }
            );
        } else {
            if (perform()) this->onClose(sender);
        }
    }

    bool reoffsetObjects(CCObject* object, std::vector<int> layersToAffect, int reoffsetValue, bool editorLayer2, int& affectedObjects) {
        auto* gameObject = typeinfo_cast<GameObject*>(object);

        if (!gameObject) {
            auto* startposObject = typeinfo_cast<StartPosObject*>(object);
            if (!startposObject) {
                FLAlertLayer::create("Error", "Unknown object type found. Please let the developer know about this bug.", "OK")->show();
                return false;
            }
            
            if (editorLayer2) {
                if (vecContains(layersToAffect, startposObject->m_editorLayer2)) {
                    if (startposObject->m_editorLayer2 + reoffsetValue > 32767) {
                        startposObject->m_editorLayer2 = 32767;
                    } else if (startposObject->m_editorLayer2 + reoffsetValue < 0) {
                        startposObject->m_editorLayer2 = 0;
                    } else {
                        startposObject->m_editorLayer2 += reoffsetValue;
                    }

                    affectedObjects++;
                }
            } else {
                if (vecContains(layersToAffect, startposObject->m_editorLayer)) {
                    if (startposObject->m_editorLayer + reoffsetValue > 32767) {
                        startposObject->m_editorLayer = 32767;
                    } else if (startposObject->m_editorLayer + reoffsetValue < 0) {
                        startposObject->m_editorLayer = 0;
                    } else {
                        startposObject->m_editorLayer += reoffsetValue;
                    }

                    affectedObjects++;
                }
            }

            return true;
        }

        if (editorLayer2) {
            if (vecContains(layersToAffect, gameObject->m_editorLayer2)) {
                if (gameObject->m_editorLayer2 + reoffsetValue > 32767) {
                    gameObject->m_editorLayer2 = 32767;
                } else if (gameObject->m_editorLayer + reoffsetValue < 0) {
                    gameObject->m_editorLayer2 = 0;
                } else {
                    gameObject->m_editorLayer2 += reoffsetValue;
                }

                affectedObjects++;
            }
        } else {
            if (vecContains(layersToAffect, gameObject->m_editorLayer)) {
                if (gameObject->m_editorLayer + reoffsetValue > 32767) {
                    gameObject->m_editorLayer = 32767;
                } else if (gameObject->m_editorLayer + reoffsetValue < 0) {
                    gameObject->m_editorLayer = 0;
                } else {
                    gameObject->m_editorLayer += reoffsetValue;
                }

                affectedObjects++;
            }
        }

        return true;
    }

    bool perform() {
        bool layerFieldGood = true;
        bool reoffsetFieldGood = true;
        std::vector<int> layersToAffect;
        int affectedObjects = 0;

        auto lel = LevelEditorLayer::get();

        auto layersFieldValue = this->layersField->getString();
        auto reoffsetCountFieldValue = this->reoffsetCountField->getString();
        auto editorLayer2 = this->editorLayer2Toggle->isToggled();
        auto onlySelected = this->onlySelectedToggle->isToggled();

        // validating layers field
        layersFieldValue = geode::utils::string::replace(layersFieldValue, " ", "");
        auto layersFields = geode::utils::string::split(layersFieldValue, ",");

        for (std::string& layerFieldVal : layersFields) {
            // assuming this layer value is a range
            if (geode::utils::string::count(layerFieldVal, '-') > 0) {
                auto layerRange = geode::utils::string::split(layerFieldVal, "-");

                // if 2+ hyphens = bad
                if (geode::utils::string::count(layerFieldVal, '-') > 1) {
                    layerFieldGood = false;
                    break;
                } else {
                    // if the range contains empty elements = bad
                    if (std::any_of(layerRange.begin(), layerRange.end(), [](const std::string& s) {
                        return s.empty();
                    })) {
                        layerFieldGood = false;
                        break;
                    }
                }

                auto layerRangeBeginRes = geode::utils::numFromString<int>(layerRange[0]);
                auto layerRangeEndRes = geode::utils::numFromString<int>(layerRange[1]);

                if (!layerRangeBeginRes || !layerRangeEndRes) {
                    layerFieldGood = false;
                    break;
                }

                int layerRangeBegin = *layerRangeBeginRes;
                int layerRangeEnd = *layerRangeEndRes;

                // if layer range begin greater than its end = bad
                if (layerRangeBegin >= layerRangeEnd) {
                    layerFieldGood = false;
                    break;
                }

                for (int i = layerRangeBegin; i <= layerRangeEnd; ++i) {
                    // if layer value is negative = bad
                    // ALSO bad if it's above short upper limit
                    if ((i < 0) || (i > 32767)) {
                        layerFieldGood = false;
                        break;
                    }

                    // if the layer isn't added = add it
                    if (!vecContains(layersToAffect, i)) layersToAffect.push_back(i);
                }
                
                if (!layerFieldGood) break;

            // if it's a normal layer value
            } else {
                auto layerRes = geode::utils::numFromString<int>(layerFieldVal);
                if (!layerRes) {
                    layerFieldGood = false;
                    break;
                }

                int layer = *layerRes;

                // if layer is negative = bad
                // ALSO bad if it's above short upper limit
                if ((layer < 0) || (layer > 32767)) {
                    layerFieldGood = false;
                    break;
                }


                // else = layerFieldGood
                if (!vecContains(layersToAffect, layer)) layersToAffect.push_back(layer);
            }
        }

        if (!layerFieldGood) {
            geode::Notification::create("The layer field is invalid.\nPlease ensure their format follows the examples' format.\n(ex. 0, 1, 4-6, 7-9)", NotificationIcon::Error, 3.f)->show();
            return false;
        }

        // validating reoffset field (nothing really to validate but saving the boolean for probable future updates)
        // i was wrong lol there IS something to validate
        reoffsetCountFieldValue = geode::utils::string::replace(reoffsetCountFieldValue, " ", "");

        // if string has 2+ signs or if it has a sign but it's placed incorrectly = bad (why would someone ever do that)
        if ((geode::utils::string::count(reoffsetCountFieldValue, '-') > 1) | ((geode::utils::string::count(reoffsetCountFieldValue, '-') == 1) && (reoffsetCountFieldValue[0] != '-'))) {
            reoffsetFieldGood = false;
        }
        
        // trying to parse the number from the field
        auto reoffsetValueRes = geode::utils::numFromString<int>(reoffsetCountFieldValue);

        if (!reoffsetValueRes) reoffsetFieldGood = false;

        if (!reoffsetFieldGood) {
            geode::Notification::create("The reoffset field is invalid.\nPlease ensure its format follows the examples' format.\n(ex. 1, -1, 2, -3)", NotificationIcon::Error, 3.f)->show();
            return false;
        }

        int reoffsetValue = *reoffsetValueRes;

        // ensure the value is in the acceptable range
        if ((reoffsetValue > 32767) || (reoffsetValue < -32767)) {
            geode::Notification::create("Please ensure the reoffset value is in\nthe range between -32767 and 32767.", NotificationIcon::Error, 3.f)->show();
            return false;
        }
        
        auto objectsToSearchFor = lel->getAllObjects();
        if (onlySelected) objectsToSearchFor = lel->m_editorUI->getSelectedObjects();

        log::debug("Reoffsetting layers {} by {}", fmt::join(layersToAffect, ", "), reoffsetValue);

        for (int i = 0 ; i < objectsToSearchFor->count() ; ++i) {
            if (!this->reoffsetObjects(objectsToSearchFor->objectAtIndex(i), layersToAffect, reoffsetValue, editorLayer2, affectedObjects)) break;
        }

        log::debug("{} objects were affected", affectedObjects);

        if (affectedObjects <= 0) {
            geode::Notification::create("No layers were affected (maybe they are empty?)", NotificationIcon::Info, 1.5f)->show();
        } else {
            geode::Notification::create("Specified layers were reoffseted", NotificationIcon::Success, 1.5f)->show();
        }

        return true;
        
    }

public:
    TextInput* layersField;
    TextInput* reoffsetCountField;
    CCMenuItemToggler* editorLayer2Toggle;
    CCMenuItemToggler* onlySelectedToggle;

    static ReoffsetLayersPopup* create() {
        auto ret = new ReoffsetLayersPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};


class $modify(MyEditorUI, EditorUI) {
	$override
	bool init(LevelEditorLayer* editorLayer) {
		if (!EditorUI::init(editorLayer)) {
			return false;
		}

		this->createKeybinds();
		return true;
	}

    void createMoveMenu() {
		EditorUI::createMoveMenu();

		auto* btn = this->getSpriteButton("reoffset-button.png"_spr, menu_selector(MyEditorUI::onButtonClick), nullptr, 0.9f);
        btn->setID("reoffset-button"_spr);
		m_editButtonBar->m_buttonArray->addObject(btn);

		auto rows = GameManager::sharedState()->getIntGameVariable("0049");
		auto cols = GameManager::sharedState()->getIntGameVariable("0050");
		
		m_editButtonBar->reloadItems(rows, cols);
	}

    void onButtonClick(CCObject* sender) {
        this->openReoffsetLayersPopup();
    }

    void openReoffsetLayersPopup() {
        auto popup = ReoffsetLayersPopup::create();
        popup->m_noElasticity = true;
        popup->show();
    }

    void createKeybinds() {
        this->addEventListener(
            KeybindSettingPressedEventV3(Mod::get(), "reoffset-layers-keybind"),
            [this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
                if (down && !repeat) {
                    openReoffsetLayersPopup();
                }
            }
        );
    }
};