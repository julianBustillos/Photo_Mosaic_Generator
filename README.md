# Photo Mosaic Generator
An easy-to-use command line executable that generates a photo mosaic based on a main image and a photo database.

## Description
This project goal is to generate a photo mosaic based on:
* **a main image** that the mosaic has to match following several visual criterions.
* **an image database** that is used for tiling the mosaic. The more image there is in this database, the better the output result quality will be.
The result can then be watched from close distance, were only the tiles will be recognizable, or from great distance where only the main image shape will be recognized. 
The main idea behind this project is to offer a free and accessible software for everyone to generate that kind of images.

## Getting Started
### Dependencies
* Windows OS

### Installing
* Unzip the latest release file

### Executing program
* Find the image you want the mosaic to match 
* Create a folder containing all the tile candidate images
* Choose the number of subdivisions you need for the mosaic
* Launch the executable with appropriate arguments.
* Example:
```
Photo_Mosaic_Generator.exe --photo match.jpg --tiles tiles_folder --subdiv 40
```

## Help
For further information and options overview, you can use the help option:
```
Photo_Mosaic_Generator.exe --help
```

## Authors
Julian Bustillos - [bustillosjulian@gmail.com](mailto:bustillosjulian@gmail.com)

## Version History
* 1.0
  * Initial Release

## License
This project is licensed under the GNU GPLv3 License - see the LICENSE.md file for details.

## Acknowledgments
All the articles that inspired the project are contained in the *articles* folder in source code.
