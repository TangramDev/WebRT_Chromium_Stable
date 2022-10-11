<h1 align=center>
Working with <i>Latest Chromium Stable Source Code</i>
</h1>
<h2  align=left><p>

We designed [OpenWebRuntime](https://github.com/TangramDev/OpenWebRunTime) to work in sync with the latest Chromium Stable Source Code for Chromium Stable Version. To do this, you need to refer to the 
<div align=center>

[**Building Chromium for Windows**](https://chromium.googlesource.com/chromium/src/+/main/docs/windows_build_instructions.md)</div>

to obtain the source code of the latest full version of the Chromium Project, and to ensure that this version can be compiled correctly so that the compiled results can run properly. The IDE Environment we work here is Visual Studio 2022 17.3.5, which requires C++/CLI support.</p>
</p> 
</h2>
<h1 align=center>
Preparation: <i>Download Chromium Source Code Patch</i>
</h1>
<h2  align=left><p>You'll need to download the latest Chromium Stable Patch zip package. 
<div align=center>

[**<i><ins>Stable Patch</ins></i>**](https://github.com/TangramDev/WebRT_Chromium_Stable/archive/refs/heads/main.zip)
</div>
  
Unzip the Patch zip you downloaded.</p>
</p> 
</h2>

<h1 align=center>
How to Use <i>

[ChromiumVer.txt](https://github.com/TangramDev/WebRT_Chromium_Canary/blob/main/ChromiumVer.txt)</i>
</h1>
<h2><p>Each Chromium WebRT Patch contains a file "ChromiumVer.txt" to specify the Tag of the Chromium Project version corresponding to the Patch, for example: 106.0.5249.99.</p>
</h2>

<h1 align=center>
How to use <i>Batch Files</i> in Chromium WebRT Patch</i>
</h1>
<h2><p>Each Chromium WebRT Patch contains a group of batch files to handle the various work done by WebRuntime for the Chromium Project, including necessary source code modifications, toolchain adjustments, project compilation, and code synchronization with the Chromium Project.</p>
</h2>
