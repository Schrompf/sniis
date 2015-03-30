# sniis
Simple Non-Intrusive Input System

What it does:

- Queries inputs from Desktop controls such as keyboards, mice, controllers, joysticks
- Supports multiple mice and keyboards (currently Windows only)
- Supports event mapping to rebind controls optionally
- Does NOT take over the main message loop of your game.
- Minimal C++ build - just add a few files to your project / makefile, include a single file

Where does it come from:

- Based on OIS (https://sourceforge.net/projects/wgois/)
- Extended with XInput patch from there
- Extended to use RawInput on Windows

Why does it look like it looks:

- I required an Input system which does not do message handling itsself
- I needed support for multiple mice/keyboards
- I needed event remapping
- I dislike RegisterAbstractFactoryAdaptorMutatorDelegateVisitor pattern misuse.
- I dislike CMake, even though I concur it's a nasty solution to the ridiculous mess that is called C++ build system.

What you can do with it:

- Whatever you want
- But don't blame me if something went wrong
- That's the actual license
- Buy me a beverage and tell me about your project 
- That's an optional side quest
