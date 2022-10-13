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

How to Use <i>[ChromiumVer.txt](https://github.com/TangramDev/WebRT_Chromium_Stable/blob/main/ChromiumVer.txt)</i>
</h1>
<h2><p>Each Chromium WebRT Patch contains a file "ChromiumVer.txt" to specify the Tag of the Chromium Project version corresponding to the Patch, for example: 106.0.5249.99.</p>
</h2>

<h1 align=center>
How to use <i>Batch Files</i> in Chromium WebRT Patch</i>
</h1>
<h2><p>Each Chromium WebRT Patch contains a group of batch files to handle the various work done by WebRuntime for the Chromium Project, including necessary source code modifications, toolchain adjustments, project compilation, and code synchronization with the Chromium Project.</p>
</h2>

<h2 align=center>

How to Use <i>[GetBranch.Bat](https://github.com/TangramDev/WebRT_Chromium_Stable/blob/main/ChromiumSRC/getbranch.bat)</i>

<p align=left>

<div align=left>

The role of GetBranch is to create a new source code branch based on the Tag value of the given Chromium Project version. GetBranch.Bat requires two parameters. The first parameter is the Tag value provided by [<i>ChromiumVer.txt</i>](https://github.com/TangramDev/WebRT_Chromium_Stable/blob/main/ChromiumVer.txt), and the second parameter is the name of the new branch you wish to create.</div></p>
</h2>

<h2 align=center>

How to Use <i>[GetWebRTbranch.Bat](https://github.com/TangramDev/WebRT_Chromium_Stable/blob/main/ChromiumSRC/getWebRTbranch.bat)</i>

<p align=left>

<div align=left>What GetWebRTbranch does is to create a new <i>WebRuntime-Enabled</i> source code Branch based on the Tag value for a given Chromium Project version. GetWebRTbranch.Bat requires three parameters. The first parameter is the Tag value of the Chromium version expected by the new branch you want to create, the second parameter is the name of the new branch you want to create, and the third parameter is the name of a code branch that already supports WebRuntime</div></p>
</h2>

<h1 align=center>

Merge the [Chromium WebRT Patch](https://github.com/TangramDev/WebRT_Chromium_Stable/archive/refs/heads/main.zip) into your <i>Chromium Project Source Code</i>
</h1>
<h2 align=center>
<p align=left>

<div align=left>If you have successfully fetched the source code of the Chromium Project, assuming that the path of the folder where your source code is located is "d:\webrt\m108", we first need to create a "WebRT base branch", you need to Give this branch an appropriate name, eg "WebRTBase".</div></p>

<div align=left>
	
Unzip the [Chromium WebRT Patch Archive](https://github.com/TangramDev/WebRT_Chromium_Stable/archive/refs/heads/main.zip), and then copy all the batch files in it to the chromium source code folder, as shown in the following figure:	
</div>

<div align=center><img src="https://user-images.githubusercontent.com/26355688/195234682-9a78ef26-6e19-47b9-85ed-5019241e4327.png" width="75%"/></div>
<div align=left>
<p>After copying all batch files,You need to open "cmd.exe" as administrator, and execute the following command:

	$ cd/d d:\webrt\m108\src

as shown in the following figure:</p>
<div align=center>

![image](https://user-images.githubusercontent.com/26355688/195236900-369331ae-a914-4537-9340-5292fb1c86f2.png)</div>

<p>execute the following command:

	$ ..\getbranch y WebRTBase
</p>
<p>
	
Here, 106.0.5249.y is the Tag value contained in <i>[ChromiumVer.txt](https://github.com/TangramDev/WebRT_Chromium_Stable/blob/main/ChromiumVer.txt)</i>, and WebRTBase is the branch name of "WebRT Base Branch"</p>

<p>Copy the folder "ChromiumSRC\src" (this folder is included in the unzipped folder of Chromium WebRT Patch) to "d:\WebRT\M108\src", execute the following command:
	
	$ git add . && git commit -am "WebRuntime Support"
</p>
<p>we have completed the merging of the patch package into the Chromium source code.</p>
</h2>

<h1 align=center>

WebRuntime based on<br/> a <i>Specific Chromium Project Stable Version: 106.0.5249.x</i>
</h1>
<h2>
<p>Execute the following command:
	
	$ ..\getWebRTbranch x your_branch_name WebRTBase
Here, WebRTBase is the source code branch that supports WebRuntime created in the above steps, and x is an integer, you will obtain a branch with webruntime support you expected.
</p>
</h2>

<h1 align=center>

Generate <i>Visual Studio</i> Solution
</h1>
<h2>
<p>Execute the following command:
	
	$ ..\gb
Normally, you need to delete the default subfolder of the out folder
</p>
</h2>

<h1 align=center>

Compiling <i>Chromium with WebRuntime Support</i>
</h1>
<h2>
<p>Execute the following command:
	
	$ ..\bd tasknumber
Normally, tasknumber=2*cpucorenumber+2.
</p>
</h2>
