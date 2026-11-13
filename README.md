# Projet OCR - WOOHP

## General Information

*This gadget (tool) is provided by the WOOHP.*

### Purpose of the project

The OCR project has a main goal to use a given image file and solve the words puzzle given as arguments in command lines.

### Procedure

To proceed, the user must open the graphical interface with the command *[COMMAND]*.
Then, the graphical interface is easy to use. There are the following implementations :

 - A "file" button to import a new image.
 - A "help" button to explain the interface in detail.
 - An "about" button to redirect the user to this page.
 - *[EXPAND LATER]*

> Note that the image imported will be replaced by the new one you import.

## Process details

### First steps

#### Manually

Once the image is imported in the interface, the user can manually rotate the image and center the grid in a given area.
If not done manually, this process of selecting the grid area will be done automatically by the program.

#### Automatically

> **The automatic mode is not available in the early version of the tool.**

Notable processes :

 - The image is rotated to be placed straight.
 - The image will be changed to be fully gray.
 - A noise reduction will be applied.
 - Enhancement done to the image.

### Divide to conquer

After pretreatment, the image will be split in two processes :

#### Word list

The word list will be located outside the grid, and split to make each word its own array later.

#### Grid

The grid may be implicit, but the scripts solve the issue of not finding the grid lines.
The grid is split in cells, each cell will be kept as a picture.
Each letter in the words will be split and be ordered.

## Neural Network

The neural network will have been trained beforehand, and is trained enough to recognise most characters.
Its goal is to spot characters and assign them a letter in the solver.

### Recognition

As each letter is kept as a picture, and ordered, the neural network will associate the correct letter to the image and thus store the letters in an array.

Each word will be analysed too.

## Solver

With each word read and each cell recognised, the solver will iterate through the grid and solve the puzzle.

### Graphical solving

To make it understandable by a human, the solver will outline the found words on the image, and save the solved puzzle in an image.

This image will be available once the image has been processed.

## Conclusion and precisions

This file will be updated regularely.
If you have suggestions, feel free to contact us, (*see AUTHORS file*) and we hope our tool could help you solve your puzzles !

"La bise"

*The spies and Jerry*
