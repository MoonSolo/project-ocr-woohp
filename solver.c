#include <stdio.h>
#include <strings.h>
#include <err.h>
#include "solver.h"

void count_lines_columns(const char *filepath, int dimensions[2])
{
    FILE *input = fopen(filepath,"r");

    if (input == NULL)
        errx(1,"Error! Could not open file\n");

    int ch;
    int curr_col = 0;
    int max_cols = 0;
    int lines = 0;

    while((ch = fgetc(input)) != EOF)
    {
        if (ch == '\n')
        {
            lines++;
            if (curr_col > max_cols)
                max_cols = curr_col;
            curr_col = 0;
        }
        else curr_col++;
    }

    if (curr_col > 0 || lines == 0)
    {
        lines++;
        if (curr_col > max_cols)
            max_cols = curr_col;
    }
    fclose(input);

    dimensions[0] = lines;
    dimensions[1] = max_cols;
}

//Lecture du fichier filepath généré par l'IA précédement
//Renvoie un array d'arrays de strings représentant le crossword
void draw_crossword(const char *filepath,
                    const int lines,
                    const int columns,
                    char crossword[lines][columns])
{
    FILE *input = fopen(filepath, "r");
    if (input == NULL)
    {
        errx(1,"Error! Could not open file\n");
    }

    int ch;
    int i = 0;
    int j = 0;
    while((ch = fgetc(input)) != EOF)
    {
        if (ch == '\n')
        {
            crossword[j][i] = '\0';
            i = 0;
            j++;
        }
        else
        {
            crossword[j][i] = (char)ch;
            i++;
        }
    }

    if (i > 0)
        crossword[j][i] = '\0';

    fclose(input);
}

int count_length(const char *word)
{
    int length = 0;
    while(*word)
    {
        length++;
        word++;
    }

    return length;
}


// The function returns the end coordinates of the word if it finds it on the
// crossword, [-1,-1] if not.
void check_word(const char *word,
                int coords[2][2],
                const int x,
                const int y,
                const int length,
                const int lines,
                const int columns,
                const char crossword[lines][columns])
{
    int i = x;
    int j = y;
    int letterWord = 0;

    coords[1][0] = -1;
    coords[1][1] = -1;

    //Check all the South coordinates
    if (j + length <= lines)
    {
        //Check the South
        while(j < lines)
        {
            if (crossword[j][i] != word[letterWord])
            {
                break;
            }
            j++;
            letterWord++;
        }

        if (letterWord == length)
        {
            j--;

            coords[0][0] = x;
            coords[0][1] = y;

            coords[1][1] = i;
            coords[1][0] = j;
            return;
        }

        i = x;
        j = y;
        letterWord = 0;

        //Check the South-East
        if (i + length <= columns)
        {
            while(j < lines && i < columns)
            {
                if (crossword[j][i] != word[letterWord])
                    break;
                j++;
                i++;
                letterWord++;
            }

            if (letterWord == length)
            {
                i--;
                j--;

                coords[0][0] = x;
                coords[0][1] = y;

                coords[1][1] = i;
                coords[1][0] = j;
                return;
            }
        }

        i = x;
        j = y;
        letterWord = 0;

        //Check the South-West
        if (i - length >= 0)
        {
            while(j < lines && i >= 0)
            {
                if (crossword[j][i] != word[letterWord])
                    break;
                j++;
                i--;
                letterWord++;
            }

            if (letterWord == length)
            {
                i++;
                j--;

                coords[0][0] = x;
                coords[0][1] = y;

                coords[1][1] = i;
                coords[1][0] = j;
                return;
            }
        }
        i = x;
        j = y;
        letterWord = 0;
    }

    //Check all the North coordinates
    if (j - length >= -1)
    {
        //Check the North
        while(j >= 0)
        {
            if (crossword[j][i] != word[letterWord])
                 break;

            j--;
            letterWord++;
        }

        if (letterWord == length)
        {
            j++;

            coords[0][0] = x;
            coords[0][1] = y;

            coords[1][1] = i;
            coords[1][0] = j;
            return;
        }

        i = x;
        j = y;
        letterWord = 0;

        //Check the North-East
        if (i + length <= columns)
        {
            while(j >= 0 && i < columns)
            {
                if (crossword[j][i] != word[letterWord])
                    break;
                j--;
                i++;
                letterWord++;
            }

            if (letterWord == length)
            {
                i--;
                j++;
                coords[0][0] = x;
                coords[0][1] = y;

                coords[1][1] = i;
                coords[1][0] = j;
                return;
            }
        }

        i = x;
        j = y;
        letterWord = 0;

        //Check the North-West
        if (i - length >= -1)
        {
            while(j >= 0 && i >= 0)
            {
                if (crossword[j][i] != word[letterWord])
                    break;
                j--;
                i--;
                letterWord++;
            }

            if (letterWord == length)
            {
                i++;
                j++;

                coords[0][0] = x;
                coords[0][1] = y;

                coords[1][1] = i;
                coords[1][0] = j;
                return;
            }
        }

        i = x;
        j = y;
        letterWord = 0;
    }

    //Check the East
    if (i + length <= columns)
    {
        while(i < columns)
        {
            if (crossword[j][i] != word[letterWord])
                break;

            i++;
            letterWord++;
        }

        if (letterWord == length)
        {
            i--;

            coords[0][0] = x;
            coords[0][1] = y;

            coords[1][1] = i;
            coords[1][0] = j;
            return;
        }
        i = x;
        j = y;
        letterWord = 0;
    }

    //Check the West
    if (i - length >= 0)
    {
        while(i >= 0)
        {
            if (crossword[j][i] != word[letterWord])
                break;
            i--;
            letterWord++;
        }
        if (letterWord == length)
        {
            i++;

            coords[0][0] = x;
            coords[0][1] = y;

            coords[1][1] = i;
            coords[1][0] = j;
            return;
        }
    }

}

void search_word(const char *word,
                 const int lines,
                 const int columns,
                 int coords[2][2],
                 const char crossword[lines][columns])
{
    if (word == NULL)
        errx(1,"Invalid word");


    int x = 0;
    int y = 0;

    int length = count_length(word);
    char firstLetter = word[0];

    while(y < lines)
    {
        x = 0;
        while(x < columns)
        {
            if (crossword[y][x] == firstLetter)
            {
            check_word(word, coords, x, y,
                           length, lines, columns,crossword);
                if (coords[1][1] != -1)
                    return;
            }
            x++;
        }
        y++;
    }
}


int main(int argc, char *argv[])
{
    if (argc < 3)
        errx(1, "Invalid usage. Usage is :\n"
                "%s <file> <word>\n", argv[0]);
    if (argc > 4)
        errx(1, "Cannot search multiple words simultaneously.");

    const char *filepath = argv[1];
    const char *word = argv[2];

    int dimensions[2] = {0,0}; // dimensions[0] -> lines |
                               // dimensions[1] -> columns

    count_lines_columns(filepath, dimensions);


    int lines = dimensions[0];
    int columns = dimensions[1];

    int coords[2][2] = {{-1,-1},{-1,-1}};

    char crossword[lines][columns];

    draw_crossword(filepath, lines, columns, crossword);

    search_word(word, lines, columns, coords,crossword);

    if (coords[1][0] == -1)
    {
        printf("Word not found\n");
    }
    else
    {
        printf("(%d,%d)(%d,%d)\n",coords[0][0],coords[0][1],
                                coords[1][1],coords[1][0]);
    }

    return 0;

}


