// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolMenus.h"
#include "AssetRegistry/AssetData.h"

#include "Core/BPR_Core.h"
#include "UI/BPR_OutputWindow.h"
#include "UI/BPR_InfoWindow.h"

class FBPR_ContentBrowserAssetActions
{
public:
    void Register();
    void Unregister();

    /** Инициализация зависимостей из модуля (вызвать один раз при старте) */
    void SetCore(TSharedPtr<BPR_Core> InCore);
    void SetOutputWindow(TSharedPtr<BPR_OutputWindow> InOutputWindow);

    /** Click handler */
    void OnShowBPAsMDClicked();

private:
    UObject* GetSelectedAsset() const;
    
    /** Основная логика обработки выбранного ассета */
    void ExecuteForObject(UObject* SelectedObject);

private:
    TWeakPtr<BPR_Core> CoreInstance;
    TWeakPtr<BPR_OutputWindow> OutputWindow;
};