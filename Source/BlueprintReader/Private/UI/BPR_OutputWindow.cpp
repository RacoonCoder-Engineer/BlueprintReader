// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "UI/BPR_OutputWindow.h"
#include "UI/BPR_TabSwitcher.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

BPR_OutputWindow::BPR_OutputWindow()
{
	// The constructor no longer creates a TabSwitcher via MakeShared
}

BPR_OutputWindow::~BPR_OutputWindow()
{
	// Close the window if it exists
	if (TSharedPtr<SWindow> W = Window.Pin())
	{
		W->RequestDestroyWindow();
	}
}

void BPR_OutputWindow::Open(const TOptional<FBPR_ExtractedData>& InitialData)
{
	// If the window is already open, bring it to the foreground.
	if (Window.IsValid())
	{
		Window.Pin()->BringToFront();
		return;
	}

	// Create a TabSwitcher and immediately transfer the initial data
	TSharedRef<SBPR_TabSwitcher> TabSwitcherRef = SNew(SBPR_TabSwitcher)
		.InitialData(InitialData);

	// Create a new Slate window and insert a TabSwitcher into it
	TSharedRef<SWindow> NewWindow = SNew(SWindow)
		.Title(FText::FromString("BPR Output"))
		.ClientSize(FVector2D(800, 600))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		[
			TabSwitcherRef
		];

	FSlateApplication::Get().AddWindow(NewWindow);

	// Saving links
	Window = NewWindow;
	TabSwitcher = TabSwitcherRef;

	UE_LOG(LogTemp, Log, TEXT("BPR_OutputWindow: Window opened successfully"));
}
