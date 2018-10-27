### **Guide to solving v141_xp build issue in Visual Studio 2015+**


With only the default build tools installed you might receive an error that looks like this when trying to build:

![alt text](https://github.com/CCExtractor/ccextractor/tree/master/troubleshooting/images/v141_xp-img1)

It means you are missing the correct build tools to build this project. This is an easy fix! Open Visual Studio Installer.

![alt text](https://github.com/CCExtractor/ccextractor/tree/master/troubleshooting/images/v141_xp-img2)

I have Visual Studio Community 2017, but whatever you have installed should be on the top left. Click modify. Once the new screen opens, there will be options on the right side, click Desktop development with C++.

![alt text](https://github.com/CCExtractor/ccextractor/tree/master/troubleshooting/images/v141_xp-img3)

Under optional, select the the two highlighted tools in the picture above. In the bottom right corner it will show the amount of space required for the addition, ensure you have enough space on your disk, then click Modify. If Visual Studio doesn't open up after modifying, open the ccextractor.sln again. Right click the project and click build, and this time it should be successful!

![alt text](https://github.com/CCExtractor/ccextractor/tree/master/troubleshooting/images/v141_xp-img4)