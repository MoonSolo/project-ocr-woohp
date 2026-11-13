/* In this file, we will implement the entire running script.
 * That is to say, the entire OCR project will be compilated when doing
 * "make main".
 *
 * Here is the work in order :
 *
 * Call the gui -> export image
 * Call pretreatment on exported image -> clean exported image (rotated too)
 * Delete exported image
 * Locate Grid, Words -> store in "position.(json/txt/other)"
 * Split image into multiple smaller images (each letter) -> export
 * Call Neural Network on each small image, iterate in order, store in "grid"
 * Call Neural Network on words to find -> export in "words"
 * Iterate through file "words"; call solver with grid and current word.
 * Store positions into "solution"
 * Copy initial image, rotate it.
 * Call words_circler on rotated image -> "solved"
 * Load solve into gui
 * save image option; end program.
*/

int main(void) // not implemented.
    return 0;
