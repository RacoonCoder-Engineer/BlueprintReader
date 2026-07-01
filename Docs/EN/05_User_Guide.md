# User Guide — BlueprintReader

## Installing and activating the plugin

BlueprintReader is distributed through **Fab** (Epic Games' marketplace) as an editor plugin for Unreal Engine. The product is completely free for personal use and released under the **MIT** license — the source code is available on GitHub, and you're free to read it, modify it, and build it into your own tools. For commercial/studio use, Fab lists it at a symbolic price of **$10** — this "free for personal projects, cheap for business" model was a deliberate choice by the authors and is one of the plugin's key differences from typical paid marketplace solutions.

To install the plugin:

1. Open the BlueprintReader page on Fab and click **Add to Library** (or **Add to Project** directly, if you already have the target project open in the Epic Games Launcher).
2. In the Epic Games Launcher, choose the project you want to add the plugin to and confirm the installation. The Launcher will copy the plugin into your project's `Plugins` folder (or install it as an engine plugin if you installed it globally for the engine).
3. If you got the plugin some other way — for example, by downloading an archive from GitHub — unpack it into a `Plugins/BlueprintReader` subfolder inside your project folder (right next to your `.uproject` file). The structure should look like this: `YourProject/Plugins/BlueprintReader/BlueprintReader.uplugin`.
4. Launch (or relaunch) Unreal Editor for your project. On the first launch after installation, the engine will offer to rebuild the plugin's modules — accept this; it will take anywhere from a few seconds to a couple of minutes depending on your machine.
5. Make sure the plugin is enabled: open **Edit → Plugins**, find the **Other** category, and look for **BlueprintReader** in it. The **Enabled** checkbox should be checked. The plugin is marked as `EnabledByDefault`, so you normally shouldn't need to enable it manually — but if you don't see the new context menu entry after installation, this is the checkbox to check first.

**Supported engine versions.** The plugin is built from a single source tree and supports **Unreal Engine 5.7 and 5.8** with no action required on your part — just install the version that matches your engine (on Fab these will be separate "Project Version" entries for each engine version). If you're using an older version of UE (5.6 or earlier), the plugin may fail to build — the authors don't guarantee backward compatibility with earlier releases.

**Platforms.** The plugin is officially supported on Windows, macOS, and Linux, since it's a purely editor-side C++/Slate tool that doesn't depend on your game's target platform.

Once the plugin is enabled, no further setup is needed — you can jump straight to the "Quick Start" section.

## Quick start

The idea behind BlueprintReader is deliberately simple: you right-click an asset in the Content Browser and get back a clean, readable Markdown text describing its structure, logic, and (for widgets) visual layout. You can copy this text and paste it into a conversation with any AI assistant (ChatGPT, Claude, Gemini, etc.) to get help debugging, refactoring, documenting, or simply understanding "what does this Blueprint do."

Steps for your first run:

1. Open the **Content Browser** in the editor.
2. Find any Blueprint asset you want to inspect — for example, a character's Actor Blueprint or a Widget Blueprint for a UI screen.
3. **Right-click** it to open the context menu.
4. In the menu that appears, find the **"Read Blueprint for AI-Assistant"** entry (in the asset actions section) and select it.
5. If the asset type is supported, a separate plugin window will open with tabs (for example, **Structure** and **Graph**, plus **Design** for widgets). If the asset isn't supported yet, you'll instead see a small info window explaining this, with a link to the project's GitHub repository where you can check for updates.
6. Inside the window, switch between the tabs to see different slices of information about the asset: the overall structure (variables, components, functions), graph logic (events, nodes, connections), and, for UMG widgets, the visual hierarchy of UI elements.
7. Once you've found the text you need, either select and copy it directly from the window, or use the **"Export to file…"** button at the bottom of the window to save the whole result to an `.md` or `.txt` file (more on this in the "Exporting results" section).
8. Paste the resulting text into a chat with your AI assistant along with your question — for example, "explain what this Blueprint does" or "help me find the cause of a bug in this logic."

An important point: BlueprintReader **never sends anything anywhere on its own**. It doesn't connect to any LLM APIs and doesn't require any access keys — this is a deliberate architectural decision by the authors. The plugin only prepares clean, compact text; the choice of which AI tool to paste that text into is entirely up to you.

## Supported asset types

At the moment, BlueprintReader is able to "read" the following asset types:

- **Actor Blueprints** — any Blueprint class inherited from `Actor` (characters, interactive objects, pickups, etc.).
- **ActorComponent Blueprints** — Blueprint classes for components you attach to actors.
- **Widget Blueprints (UMG)** — screens and UI elements, including both "leaf" widgets and reusable user widgets (`WBP_*`) nested inside one another.
- **Blueprint Interfaces (BPI)** — Blueprint interfaces describing a set of contract functions without implementation.
- **User-Defined Enums** — enumerations created through the editor (not C++ enums).
- **User-Defined Structures** — custom data structures created through the editor.
- **Materials** — regular materials and material instances.
- **Material Functions** — material functions and their instances, i.e., reusable pieces of material logic.

For each of these types, the plugin shows a different set of tabs, because the meaning of the information differs:

- For **Widget Blueprint**, three tabs are available: **Design** (the screen's visual layout — which elements are where, their sizes, and key visual properties), **Structure** (variables, event bindings), and **Graph** (logic in event graphs and functions). The Design tab opens first, because for UI it's usually "what's drawn where" that matters most.
- For **Actor**, **ActorComponent**, **Material**, **Material Function**, and **Blueprint Interface**, two tabs are available: **Structure** and **Graph**.
- For **Enum** and **Structure**, only one tab is available — **Structure** — since these asset types simply have no graph logic or visual layout.

**What the plugin doesn't support yet.** If you try to open, for example, a plain non-actor, non-component Blueprint without an explicit subtype, an Anim Blueprint, a sound asset, a texture, a Data Asset, a Data Table, a Niagara system, or a level asset — the plugin will honestly tell you that this type isn't implemented yet, rather than showing an empty or incorrect window. This is intentional: it's better to clearly say "not supported yet" than to produce a misleading result. The list of supported types will keep growing in future versions — you can follow updates on the project's GitHub page, which is linked directly from the popup window shown for unsupported assets.

## Plugin settings

BlueprintReader's settings live in the standard place for Unreal Engine plugins: **Edit → Project Settings → Plugins → Blueprint Reader**. Three parameters are available there:

- **Export Format** — the format the file will be saved in when exporting results: **Markdown (.md)** or **Plain Text (.txt)**. Markdown is selected by default — it's better suited for pasting into AI assistant chats, since most of them correctly recognize Markdown formatting (headings, lists, code blocks) and render it readably. Plain Text is useful if you need maximally "raw" text without any markup — for example, for pasting into systems that don't support Markdown.
- **Restrict Widget Recursion Depth** — turns the depth limit for traversing nested widgets on or off. Enabled by default.
- **Widget Recursion Depth** — the actual depth value, from 1 to 50 (default 10). This parameter only becomes editable when the checkbox above is enabled.

Why does this depth setting exist? UMG widgets are often nested inside one another: a screen contains a reusable character card widget, which contains a health bar widget, which in turn contains other elements, and so on. If the plugin were allowed to unfold this nesting infinitely, the result for a complex project could turn into a huge "wall" of text that's uncomfortable to read and expensive to paste into a conversation with an AI assistant (most chat models have a practical limit on how much context fits in a single message). The depth limit is a sensible compromise: the plugin unfolds widget nesting up to a given level, and beyond that it simply stops going deeper, keeping the structure compact and readable.

**How to choose a value.** If you're working with a simple UI (a couple of nesting levels), the default value (10) comfortably covers any real-world screen, and you'll most likely never run into the limit in practice. If, on the other hand, you're building something with very deep nesting of reusable widgets and notice that the output cuts off earlier than expected, increase the value or uncheck **Restrict Widget Recursion Depth** entirely to remove the limit altogether. The opposite scenario is also possible: if you're only interested in the top-level layout of a screen without deep details of nested widgets, you can instead decrease the depth — this will make the output shorter and more focused.

Changes to the settings take effect immediately on the next data extraction run — there's no need to restart the editor.

## Exporting results

Besides reading the result directly in the plugin window, you can save it to a file — this is handy if you want to:

- attach the file to a bug tracker ticket or a message to a colleague;
- save a "snapshot" of a Blueprint's state at a given point in time, to compare later with a new version;
- paste the file's contents into an AI tool that accepts files rather than just chat text (for example, when working with assistants built into an IDE).

For this, there's an **"Export to file…"** button at the bottom of the plugin window. It's visible on every tab — the panel with the button doesn't disappear when switching between Structure/Graph/Design, because it lives in the window's shared "footer."

Clicking the button does the following:

1. The plugin gathers together all the meaningful sections of the result (Structure, Graph, Design — for whichever tabs apply to the current asset type), skipping sections that are empty or not applicable to this asset type (for example, a material won't have a Design section, so it simply won't end up in the file).
2. If there's genuinely nothing to save, you'll see a **"Nothing to export for this asset."** notification, and the save dialog won't open.
3. If there is something to save, the standard system **"Export Blueprint Data"** window opens for choosing the location and file name. The file extension (`.md` or `.txt`) and the file type filter in this dialog are automatically set according to the **Export Format** parameter from the plugin settings (see the previous section).
4. By default, the suggested file name is taken from the asset's own name (for example, `BP_MyCharacter.md`), so in most cases you can simply click "Save" without renaming anything.
5. If you change your mind and close the save dialog without picking a file, nothing happens — there's no error notification, since this is normal behavior, not a failure.
6. After a successful save, you'll see a success notification with the file name. If something went wrong (for example, insufficient permission to write to the chosen folder), you'll see an export error notification.

The file is saved in UTF-8 encoding, so Cyrillic text, special characters, and any non-standard variable names from your project display correctly in any text editor.

## Frequently Asked Questions (FAQ)

**Is the plugin really free?**
Yes. BlueprintReader is an open-source project under the MIT license, available on GitHub. On Fab it's free for personal/non-commercial use. If you use it within a studio/business, the authors ask you to purchase a license for a symbolic amount of $10 — this is more a way to support development than a full-fledged commercial model.

**Do I need an OpenAI/Anthropic/Google API key for this to work?**
No. BlueprintReader doesn't contact any external LLM services and doesn't require any access keys. It only prepares a text description of your asset — pasting that text into a specific AI assistant (in a browser, an app, an IDE) is something you do yourself, manually, with whatever tool you use. This is a fundamental architectural decision: the plugin doesn't "lock" you into a single AI provider and doesn't collect any data about where you paste the result.

**Why does clicking some assets show a small info window instead of a window with the result?**
This means the asset type isn't on the list of supported types yet (see the "Supported asset types" section). The info window says exactly that: this isn't an engine error, just functionality that hasn't been implemented yet. The window has a "Check for updates" button that leads to the project's GitHub page, where you can check whether support for the needed type has already appeared in a new version.

**Is support for Anim Blueprint, Niagara, Data Table, and other types planned?**
The list of supported types is being actively expanded. You can always check the current status and plans in the project's GitHub repository — that's where development happens in the open, and any new releases show up there first, before being synced to Fab.

**How is this different from similar plugins on Fab (for example, tools in the "Blueprint → Markdown/AI" category)?**
The main difference is openness and the distribution model: BlueprintReader is fully open-source (MIT), free for personal use, and doesn't lock you into any particular AI provider or built-in API key. On top of that, the plugin has a separate **Design** tab that specifically shows the visual layout of UMG widgets — not just graph logic, but also how the interface elements are arranged, which is especially useful when discussing UI/UX with an AI assistant.

**Can I use the exported result as official project documentation?**
Formally, yes — the file comes out readable and structured, and there's nothing stopping you from attaching it to the project wiki. But it's worth understanding that the plugin's main purpose is preparing context for an AI assistant, not replacing full-fledged manual documentation. For critical, long-lived parts of a project, we still recommend maintaining documentation written and reviewed by a human.

**Can I edit a Blueprint through this plugin?**
No. BlueprintReader is a read-only tool. It never modifies your assets in any way — it only extracts and displays information about them. Any changes to a Blueprint are still made, as before, through the standard Unreal Engine editor.

## Troubleshooting

**There's no "Read Blueprint for AI-Assistant" entry in the Content Browser context menu.**
Check that the plugin is enabled: **Edit → Plugins → Other → BlueprintReader**, the **Enabled** checkbox. If you had to check the box manually, restart the editor so the plugin's module properly hooks into the Content Browser's menu system. Also make sure you're right-clicking on an actual asset (not, say, on empty panel space or on a folder) — the menu entry is added to the context actions for a specific asset.

**Clicking the asset opens an info window about an unsupported type instead of the result window.**
This is expected behavior for assets whose type isn't yet on the list of supported types (see the "Supported asset types" section). This isn't an error — it's how the plugin tells you that extraction logic for this particular asset type hasn't been implemented yet.

**The "Export to file…" button shows "Nothing to export for this asset."**
This means none of the sections (Structure/Graph/Design) produced meaningful content for this particular asset — for example, the asset is empty, was just created and doesn't yet contain any logic, or some tabs simply don't apply to its type. Check the tab contents directly in the plugin window before exporting: if they're empty or show placeholder text like "No Data found," there's genuinely nothing to export.

**Nothing happened after clicking "Export to file…".**
If you closed the system file-selection dialog without saving (for example, clicked "Cancel"), this is normal behavior — there's no error, the plugin simply silently does nothing. If you did select a file and click "Save" but no success notification appeared, check whether you have write permission to the chosen folder (for example, try saving to your project's `Saved` folder, which is suggested by default), and check the editor's Output Log — the plugin writes a warning there if the file write fails.

**Nested widgets in the output cut off earlier than expected / deeply nested elements aren't shown.**
This is most likely a result of the **Widget Recursion Depth** setting (Edit → Project Settings → Plugins → Blueprint Reader). Increase the depth value or uncheck **Restrict Widget Recursion Depth** to remove the limit entirely — details are in the "Plugin settings" section.

**Property values of a nested custom widget (`WBP_*`) look like default values rather than what's actually set on screen.**
For nested custom widgets, the plugin shows an "Exposed Properties (set on this instance)" section — this lists properties whose values were explicitly overridden at this specific point of use (on this specific instance), as opposed to the default value for the whole widget class. If a property wasn't changed on this particular instance, it simply won't appear there, because it wouldn't carry any information beyond the default value.

**The plugin fails to build / the editor complains about the BlueprintReader module after an engine version upgrade.**
The plugin supports Unreal Engine 5.7 and 5.8 out of the box from a single source tree — no special action is required on your part. If you're using an earlier engine version (5.6 or older), the build may fail — in that case, either upgrade the engine to a supported version, or check the project's GitHub page for an older plugin version compatible with your specific engine version.

**I found a bug or want to suggest support for a new asset type — where do I go?**
Since BlueprintReader is an open-source project, the best way is to open an issue in the project's GitHub repository. You can find a link to the repository both in the popup window for unsupported assets (the "Check for updates" button) and on the product page on Fab.
