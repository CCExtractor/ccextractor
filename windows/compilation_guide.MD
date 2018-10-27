<h1>Compilation guide</h1>
<h4>*First of all you have to have your visual studio installed on your computer.</h4>

<p>When you already have visual studio you have to download ccextractor code <a href="https://github.com/CCExtractor/ccextractor/">CCExtractor</a></p>
  
<p>After downloading open your CCExtractor folder and then open windows folder.</p>
<br>
<h4>Here you can see two files .vcxproj(marked with blue arrows) and one .sln project(marked with a red arrow)</h4>

<img src="img/1.png">

<br>

<p>You have to open .sln if you want to build both non GUI and GUI ccextractor or you can build them seperately opening .vcxproj files one by one.</p>

<br>

<h4>Further, if you receive a confirmation request about downloading from unsafe resource just click OK and go forward</h4>
<br>
<img src="img/2.png">

<br>
<h4>Then if you opened .sln file you will have 2 projects on the right pannel GUI and non GUI and you can also open them separetely by opening .vcxproj files.</h4>

<img src="img/3.png">

<br>
<h4>Also there are 2 parameters in curly braces near each project(Visual studio version and Windows release version) you have to change them to parameters which are true for you by clicking right mouse button on project and selectin properties. You can see this on the photo.
</h4>
