// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "UI/BPR_InfoWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

BPR_InfoWindow::~BPR_InfoWindow()
{
    if (TSharedPtr<SWindow> W = Window.Pin())
    {
        W->RequestDestroyWindow();
    }
}

void BPR_InfoWindow::Open(const FParams& Params)
{
    // If the window is already open, bring it to the foreground
    if (Window.IsValid())
    {
        Window.Pin()->BringToFront();
        return;
    }

    // Create a new window
    TSharedRef<SWindow> NewWindow = SNew(SWindow)
        .Title(Params.Title)
        .ClientSize(FVector2D(600, 300))
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        .IsTopmostWindow(true)
        [
            SNew(SBorder)
            .Padding(10)
            [
                SNew(SVerticalBox)

                // 1 Main warning text
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 10)
                [
                    SNew(STextBlock)
                    .Text(Params.Message)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)) // рабочий шрифт
                    .ColorAndOpacity(FSlateColor(FLinearColor::Yellow))
                    .Justification(ETextJustify::Center)
                ]

                // 2 Detailed Explanation
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(20, 5)
                [
                    SNew(STextBlock)
                    .Text(Params.SubMessage)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14)) // working font
                    .ColorAndOpacity(FSlateColor(FLinearColor::White))
                    .AutoWrapText(true)
                ]

                // 3 "Check for updates" button
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(0, 20)
                [
                    SNew(SButton)
                    .Text(Params.OptionalButtonText.IsEmpty() ? FText::FromString("Check for updates") : Params.OptionalButtonText)
                    .Visibility(Params.OptionalURL.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
                    .OnClicked_Lambda([this, Url = Params.OptionalURL]()
                    {
                        if (!Url.IsEmpty())
                        {
                            FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
                        }
                        return FReply::Handled();
                    })
                ]

                // 4 Tooltip below the button
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(0, 5)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("View the roadmap and learn about plans"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
                    .ColorAndOpacity(FSlateColor(FLinearColor::Gray))
                    .Justification(ETextJustify::Center)
                ]

                // 5 OK button
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Right)
                .Padding(0, 20, 20, 10)
                [
                    SNew(SButton)
                    .Text(FText::FromString("OK"))
                    .OnClicked_Lambda([this]()
                    {
                        if (TSharedPtr<SWindow> W = Window.Pin())
                        {
                            W->RequestDestroyWindow();
                        }
                        return FReply::Handled();
                    })
                ]
            ]
        ];

    // Add a window to Slate
    FSlateApplication::Get().AddWindow(NewWindow);
    Window = NewWindow;
}


// Close the window by clicking OK
FReply BPR_InfoWindow::OnOkClicked()
{
    if (TSharedPtr<SWindow> W = Window.Pin())
    {
        W->RequestDestroyWindow();
    }
    return FReply::Handled();
}

// Open the browser when you click on the URL button
FReply BPR_InfoWindow::OnUrlClicked(const FString& Url)
{
    if (!Url.IsEmpty())
    {
        FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
    }
    return FReply::Handled();
}
