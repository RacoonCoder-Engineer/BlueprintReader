# BlueprintReader

A plugin for Unreal Engine that extracts Blueprint structure and graph information into a readable text format (Markdown-style).

Useful for documentation, code review, AI-assisted development, or simply understanding complex Blueprints at a glance.

## 📺 Video Tutorial

[YouTube How it works](https://youtu.be/vbsbJF24uAs?list=PLZuZmQd4mRCburxOj_ALrhRZdZcNWdE5e)

## Features

- **One-click extraction** — Right-click any supported asset in Content Browser → "Read Bluprint for AI Assistant";
- **Two-panel output** — Structure tab (variables, components, parameters) and Graph tab (execution flow, data connections);
- **Multiple asset types supported;**

## Installation:

Download from FAB
Extract to YourProject/Plugins/BlueprintReader/
Restart Unreal Editor
Enable plugin in Edit → Plugins → BlueprintReader

## How to Use:

Right-click any supported asset in Content Browser
Select "Read Blueprint for AI"
View results in Structure/Graph tabs
Copy text (Ctrl+A, Ctrl+C) and paste to AI assistant

## Supported Assets:

- Actor/Pawn/Character Blueprints;
- Actor Component Blueprints;
- Blueprint Interfaces;
- Materials & Material Instances;
- Material Functions & Material Function Instances;
- Enumerations;
- Structures;

## Supported Unreal Engine Versions

- Unreal Engine 5.7+

*(Earlier versions not tested, may work with minor modifications)*

## Known Limitations

- Other Classes — Work in progress  
- Very complex Blueprints with 500+ nodes may take a moment to process

## Roadmap

- [ ] Export to file (.md, .txt)
- [ ] Partly output from Graph
- [ ] Widget Blueprint support


## Support the Project

If you find this plugin useful, consider supporting its development:

🎁 **Patreon:** [Racoon Coder](https://www.patreon.com/c/u12165995)

🎁 **Boosty:** [My Boosty]()

Your support helps me dedicate more time to developing free tools for the Unreal community!

## License

[MIT License](LICENSE) — Free for personal and commercial use.

## Contact & Feedback

- **GitHub Issues:** [Report bugs or request features]()
- **Discord:** [Maybe one day...]()
- **Twitter/X:** [Maybe one day...]()

---

Made with ❤️ by Racoon Coder
