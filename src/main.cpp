#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

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
        this->editorLayer2Toggle->setPosition(center2 + ccp(85.f, 0));

        auto editorLayer2ToggleLabel = CCLabelBMFont::create("Editor layer 2", "goldFont.fnt");
        editorLayer2ToggleLabel->setScale(0.2f);
        editorLayer2ToggleLabel->setPosition(center2 + ccp(85.f, 15.f));

        auto confirmButtonSprite = ButtonSprite::create("Confirm");
        auto confirmButton = CCMenuItemSpriteExtra::create(confirmButtonSprite, this, menu_selector(ReoffsetLayersPopup::onConfirm));
        confirmButton->setPosition(center2 + ccp(0, -55));
        
        m_mainLayer->addChild(this->layersField);
        m_mainLayer->addChild(this->reoffsetCountField);
        m_mainLayer->addChild(editorLayer2ToggleLabel);

        m_buttonMenu->addChild(editorLayer2Toggle);
        m_buttonMenu->addChild(confirmButton);

        return true;
    }

    void onConfirm(CCObject* sender) {
        bool layerFieldGood = true;
        bool reoffsetFieldGood = true;
        std::vector<int> layersToAffect;

        auto lel = LevelEditorLayer::get();

        auto layersFieldValue = this->layersField->getString();
        auto reoffsetCountFieldValue = this->reoffsetCountField->getString();
        auto editorLayer2 = this->editorLayer2Toggle->isToggled();

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
                    if (i < 0) {
                        layerFieldGood = false;
                        break;
                    }

                    // if the layer isn't added = add it
                    if (std::find(layersToAffect.begin(), layersToAffect.end(), i) == layersToAffect.end()) layersToAffect.push_back(i);
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
                if (layer < 0) {
                    layerFieldGood = false;
                    break;
                }


                // else = layerFieldGood
                if (std::find(layersToAffect.begin(), layersToAffect.end(), layer) == layersToAffect.end()) layersToAffect.push_back(layer);
            }
        }

        if (!layerFieldGood) {
            geode::Notification::create("The layer field is invalid.\nPlease ensure their format follows the examples' format.\n(ex. 0, 1, 4-6, 7-9)", NotificationIcon::Error, 3.f)->show();
            return;
        }

        // validating reoffset field (nothing really to validate but saving the boolean for probable future updates)
        reoffsetCountFieldValue = geode::utils::string::replace(reoffsetCountFieldValue, " ", "");

        // if string has 2+ signs or if it has a sign but it's placed incorrectly = bad (why would someone ever do that)
        if ((geode::utils::string::count(reoffsetCountFieldValue, '-') > 1) | ((geode::utils::string::count(reoffsetCountFieldValue, '-') == 1) && (reoffsetCountFieldValue[0] != '-'))) {
            reoffsetFieldGood = false;
        }
        
        auto reoffsetValueRes = geode::utils::numFromString<int>(reoffsetCountFieldValue);

        if (!reoffsetValueRes) reoffsetFieldGood = false;

        if (!reoffsetFieldGood) {
            geode::Notification::create("The reoffset field is invalid.\nPlease ensure its format follows the examples' format.\n(ex. 1, -1, 2, -3)", NotificationIcon::Error, 3.f)->show();
            return;
        }

        int reoffsetValue = *reoffsetValueRes;
        
        auto allObjects = lel->getAllObjects();
        auto originalObjects = CCArray::create();
        
        // finding objects we need to affect
        for (int i = 0 ; i < allObjects->count() ; ++i) {
            auto* gameObject = typeinfo_cast<GameObject*>(allObjects->objectAtIndex(i));

            if (!gameObject) {
                auto* startposObject = typeinfo_cast<StartPosObject*>(allObjects->objectAtIndex(i));
				if (!startposObject) {
					FLAlertLayer::create("Error", "Unknown object type found. Please let the developer know about this bug.", "OK")->show();
					return;
				}
                
                if (editorLayer2) {
                    if (std::find(layersToAffect.begin(), layersToAffect.end(), startposObject->m_editorLayer2) != layersToAffect.end()) originalObjects->addObject(startposObject);
                } else {
                    if (std::find(layersToAffect.begin(), layersToAffect.end(), startposObject->m_editorLayer) != layersToAffect.end()) originalObjects->addObject(startposObject);
                }

                continue;
            }

            if (editorLayer2) {
                if (std::find(layersToAffect.begin(), layersToAffect.end(), gameObject->m_editorLayer2) != layersToAffect.end()) originalObjects->addObject(gameObject);
            } else {
                if (std::find(layersToAffect.begin(), layersToAffect.end(), gameObject->m_editorLayer) != layersToAffect.end()) originalObjects->addObject(gameObject);
            }
        }

        // copying all objects (because we wanna do undo action)
        std::string objectStrings = "";

        if (originalObjects->count() <= 0) {
            geode::Notification::create("No layers were affected (maybe they are empty?)", NotificationIcon::Info, 1.5f)->show();
            this->onClose(sender);
            
            return;
        }

        for (int i = 0 ; i < originalObjects->count() ; i++) {
			auto* gameObject = typeinfo_cast<GameObject*>(originalObjects->objectAtIndex(i));
				
			if (!gameObject) {
				auto* startposObject = typeinfo_cast<StartPosObject*>(originalObjects->objectAtIndex(i));
				if (!startposObject) {
					FLAlertLayer::create("Error", "Unknown object type found. Please let the developer know about this bug.", "OK")->show();
					return;
				}

                auto objectString = static_cast<std::string>(startposObject->getSaveString(GJBaseGameLayer::get())) + ";";
			    objectStrings += objectString;
                continue;
			}

			auto objectString = static_cast<std::string>(gameObject->getSaveString(GJBaseGameLayer::get())) + ";";
			objectStrings += objectString;
		}
        
        auto newObjects = lel->createObjectsFromString(gd::string(objectStrings), true, true);

        // now we can delete previous objects
        lel->m_undoObjects->addObject(UndoObject::createWithArray(originalObjects, UndoCommand::DeleteMulti));
        
        for (int i = 0 ; i < originalObjects->count() ; ++i) {
            auto* gameObject = typeinfo_cast<GameObject*>(originalObjects->objectAtIndex(i));

            if (!gameObject) {
                auto* startposObject = typeinfo_cast<StartPosObject*>(originalObjects->objectAtIndex(i));
				if (!startposObject) {
					FLAlertLayer::create("Error", "Unknown object type found. Please let the developer know about this bug.", "OK")->show();
					return;
				}

                lel->removeObject(startposObject, true);
                continue;
            }

            lel->removeObject(gameObject, true);
        }

        lel->m_editorUI->deselectAll();
        lel->m_undoObjects->addObject(UndoObject::createWithArray(originalObjects, UndoCommand::DeleteMulti));

        // now we can modify new objects
        for (int i = 0 ; i < newObjects->count() ; ++i) {
            auto* gameObject = typeinfo_cast<GameObject*>(newObjects->objectAtIndex(i));

            if (!gameObject) {
                auto* startposObject = typeinfo_cast<StartPosObject*>(newObjects->objectAtIndex(i));
				if (!startposObject) {
					FLAlertLayer::create("Error", "Unknown object type found. Please let the developer know about this bug.", "OK")->show();
					return;
				}

                if (editorLayer2) {
                    if (startposObject->m_editorLayer2 + reoffsetValue >= 0) {
                        startposObject->m_editorLayer2 = startposObject->m_editorLayer2 + reoffsetValue;
                    } else {
                        startposObject->m_editorLayer2 = 0;
                    }
                } else {
                    if (startposObject->m_editorLayer + reoffsetValue >= 0) {
                        startposObject->m_editorLayer = startposObject->m_editorLayer + reoffsetValue;
                    } else {
                        startposObject->m_editorLayer = 0;
                    }
                }

                continue;
            }

            if (editorLayer2) {
                if (gameObject->m_editorLayer2 + reoffsetValue >= 0) {
                    gameObject->m_editorLayer2 = gameObject->m_editorLayer2 + reoffsetValue;
                } else {
                    gameObject->m_editorLayer2 = 0;
                }
            } else {
                if (gameObject->m_editorLayer + reoffsetValue >= 0) {
                    gameObject->m_editorLayer = gameObject->m_editorLayer + reoffsetValue;
                } else {
                    gameObject->m_editorLayer = 0;
                }
            }   
        }

        lel->m_undoObjects->addObject(UndoObject::createWithArray(newObjects, UndoCommand::Paste));
        lel->m_editorUI->selectObjects(newObjects, true);

        geode::Notification::create("Specified layers were reoffseted", NotificationIcon::Success, 1.5f)->show();
        this->onClose(sender);
        
    }

public:
    TextInput* layersField;
    TextInput* reoffsetCountField;
    CCMenuItemToggler* editorLayer2Toggle;

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