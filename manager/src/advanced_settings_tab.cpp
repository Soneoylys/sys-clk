 /*
    sys-clk manager, a sys-clk frontend homebrew
    Copyright (C) 2019  natinusala
    Copyright (C) 2019  p-sam
    Copyright (C) 2019  m4xw

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "advanced_settings_tab.h"

#include "utils.h"

#include <sysclk.h>

AdvancedSettingsTab::AdvancedSettingsTab()
{
    // Get context
    SysClkContext context;
    Result rc = sysclkIpcGetCurrentContext(&context);

    if (R_FAILED(rc))
    {
        brls::Logger::error("Unable to get context");
        errorResult("sysclkIpcGetCurrentContext", rc);
        brls::Application::crash("Could not get the current sys-clk context, please check that it is correctly installed and enabled.");
        return;
    }

    // Create UI

    // Disclaimer
    this->addView(new brls::Label(brls::LabelStyle::REGULAR, "\uE140  Please only alter these settings if you know what you are doing.", true));

    // Temporary overrides
    this->addView(new brls::Header("Temporary overrides"));

    // CPU
    brls::SelectListItem *cpuFreqListItem = createFreqListItem(SysClkModule_CPU, context.overrideFreqs[SysClkModule_CPU] / 1000000);
    cpuFreqListItem->getValueSelectedEvent()->subscribe([](int result){
        Result rc = result == 0 ?
            sysclkIpcRemoveOverride(SysClkModule_CPU) :
            sysclkIpcSetOverride(SysClkModule_CPU, g_freq_table_hz[SysClkModule_CPU][result]);

        if (R_FAILED(rc))
        {
            brls::Logger::error("Unable to update CPU Override");
            errorResult(result == 0 ? "sysclkIpcRemoveOverride" : "sysclkIpcSetOverride",  rc);
            // TODO: Reset selected value
        }
    });

    // GPU
    brls::SelectListItem *gpuFreqListItem = createFreqListItem(SysClkModule_GPU, context.overrideFreqs[SysClkModule_GPU] / 1000000);
    gpuFreqListItem->getValueSelectedEvent()->subscribe([](int result){
        Result rc = result == 0 ?
            sysclkIpcRemoveOverride(SysClkModule_GPU) :
            sysclkIpcSetOverride(SysClkModule_GPU, g_freq_table_hz[SysClkModule_GPU][result]);

        if (R_FAILED(rc))
        {
            brls::Logger::error("Unable to update GPU Override");
            errorResult(result == 0 ? "sysclkIpcRemoveOverride" : "sysclkIpcSetOverride",  rc);
            // TODO: Reset selected value
        }
    });

    // MEM
    brls::SelectListItem *memFreqListItem = createFreqListItem(SysClkModule_MEM, context.overrideFreqs[SysClkModule_MEM] / 1000000);
    memFreqListItem->getValueSelectedEvent()->subscribe([](int result)
    {
        Result rc = result == 0 ?
            sysclkIpcRemoveOverride(SysClkModule_MEM) :
            sysclkIpcSetOverride(SysClkModule_MEM, g_freq_table_hz[SysClkModule_MEM][result]);

        if (R_FAILED(rc))
        {
            brls::Logger::error("Unable to update MEM Override");
            errorResult(result == 0 ? "sysclkIpcRemoveOverride" : "sysclkIpcSetOverride",  rc);
            // TODO: Reset selected value
        }
    });

    this->addView(cpuFreqListItem);
    this->addView(gpuFreqListItem);
    this->addView(memFreqListItem);

    // Config
    this->addView(new brls::Header("Configuration"));

    // Logging
    // TODO: add a logger view and put the button to enter it here

    // Config entries
    // TODO: add constraints to the swkbd if possible (min / max)

    sysclkIpcGetConfigValues(&this->configValues);

    for (int i = 0; i < 5; i++)
    {
        SysClkConfigValue config = (SysClkConfigValue) i;

        std::string label       = std::string(sysclkFormatConfigValue(config, true));
        std::string description = this->getDescriptionForConfig(config);
        uint64_t defaultValue   = this->configValues.values[config];

        brls::IntegerInputListItem* configItem = new brls::IntegerInputListItem(label, defaultValue, label, description);

        configItem->setReduceDescriptionSpacing(true);

        configItem->getClickEvent()->subscribe([this, configItem, config](View* view)
        {
            try
            {
                int value = std::stoi(configItem->getValue());

                // Validate the value
                if (value < 0)
                {
                    brls::Application::notify("\uE5CD Couldn't save configuration: invalid value (is negative)");
                    configItem->setValue(std::to_string(this->configValues.values[config]));
                    return;
                }

                uint64_t uvalue = (uint64_t) value;

                if (!sysclkValidConfigValue(config, uvalue))
                {
                    brls::Application::notify("\uE5CD Couldn't save configuration: invalid value");
                    configItem->setValue(std::to_string(this->configValues.values[config]));
                    return;
                }

                // Save the config
                this->configValues.values[config] = uvalue;
                sysclkIpcSetConfigValues(&this->configValues);

                brls::Application::notify("\uE14B Configuration saved");
            }
            catch(const std::exception& e)
            {
                brls::Logger::error("Unable to parse config value %s: %s", configItem->getValue().c_str(), e.what());
            }
        });

        this->addView(configItem);
    }
    
    //loop for the added 3 custom configs
    for (int i = 5; i < SysClkConfigValue_EnumMax; i++)
    {
        SysClkConfigValue config = (SysClkConfigValue) i;

        std::string label       = std::string(sysclkFormatConfigValue(config, true));
        
        //uint64_t defaultValue   = configValues.values[config];
        uint64_t defaultValue   = this->configValues.values[config];
        
        if (i==(SysClkConfigValue_EnumMax-1)) {

            // Fake profile
            brls::SelectListItem *profileListItem = createProfileListItem(label, (uint32_t) defaultValue);
            profileListItem->getValueSelectedEvent()->subscribe([this,config,profileListItem](int result)
            {

                try
                {
                    uint64_t uvalue = (uint64_t) sysclk_g_profile_table[result - 1];
                    
                    if (!sysclkValidConfigValue(config, uvalue))
                    {
                        brls::Application::notify("\uE5CD Couldn't save configuration: invalid value");
                        profileListItem->setValue(sysclkFormatProfile((SysClkProfile)this->configValues.values[config],true) );
                        return;
                    }

                    // Save the config
                    this->configValues.values[config] = uvalue;
                    sysclkIpcSetConfigValues(&this->configValues);

                    brls::Application::notify("\uE14B Configuration saved");
                }
                catch(const std::exception& e)
                {
                    brls::Logger::error("Unable to parse config value %s: %s", profileListItem->getValue().c_str(), e.what());
                }
                
                
            });

            this->addView(profileListItem);

        } else {

            bool defValue = false;
            
            if(defaultValue==1) {
                defValue = true;
            }
        
            brls::ToggleListItem* configItem2 = new brls::ToggleListItem(label, defValue, "", "Yes", "No");
            
            configItem2->getClickEvent()->subscribe([this, configItem2, config](View* view)
            {
                try
                {
                    int value = 0;
                    std::string valuestring = configItem2->getValue();
                    
                    if(valuestring == "Yes") {
                        value = 1;
                    }
                    
                    uint64_t uvalue = (uint64_t) value;

                    if (!sysclkValidConfigValue(config, uvalue))
                    {
                        brls::Application::notify("\uE5CD Couldn't save configuration: invalid value");
                        configItem2->setValue(std::to_string(this->configValues.values[config]));
                        return;
                    }

                    // Save the config
                    this->configValues.values[config] = uvalue;
                    sysclkIpcSetConfigValues(&this->configValues);

                    brls::Application::notify("\uE14B Configuration saved");
                }
                catch(const std::exception& e)
                {
                    brls::Logger::error("Unable to parse config value %s: %s", configItem2->getValue().c_str(), e.what());
                }
            });

            this->addView(configItem2);
            
        }
        
    }
    
    // Notes
    brls::Label *notes = new brls::Label(
        brls::LabelStyle::DESCRIPTION,
        "Chosen minimum profile will ensure that your device will stay at least at that minimum level. It's meant for people who have higher clocks in charging profiles and sometimes want to activate those profiles without a charger (e.g. choosing 'Official Charger' will ensure that your device's profile will remain at least at 'Official Charger' even if in reality your device is in handheld). Chosen minimum profile won't downgrade your real profile if the real profile is higher than the minimum profile (e.g. when docked choosing 'Charging' doesn't do anything (you are already at a higher profile)).",
        true
    );
    this->addView(notes);
    
}

std::string AdvancedSettingsTab::getDescriptionForConfig(SysClkConfigValue config)
{
    switch (config)
    {
        case SysClkConfigValue_CsvWriteIntervalMs:
            return "How often to update /config/sys-clk/context.csv (in milliseconds)\n\uE016  Use 0 to disable";
        case SysClkConfigValue_TempLogIntervalMs:
            return "How often to log temperatures (in milliseconds)\n\uE016  Use 0 to disable";
        case SysClkConfigValue_FreqLogIntervalMs:
            return "How often to log real frequencies (in milliseconds)\n\uE016  Use 0 to disable";
        case SysClkConfigValue_PowerLogIntervalMs:
            return "How often to log power consumption (in milliseconds)\n\uE016  Use 0 to disable";
        case SysClkConfigValue_PollingIntervalMs:
            return "How fast to check and apply profiles (in milliseconds)";
        default:
            return "";
    }
}
