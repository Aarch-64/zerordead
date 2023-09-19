#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

/**
 * @brief Estructura que representa los datos del juego en el cubo.
 */
typedef struct
{
  char *cube;               	// Puntero al cubo de juego (matriz de caracteres)
  int row;                  	// Número de filas en el cubo de juego
  int col;                  	// Número de columnas en el cubo de juego
  int current_position;     	// Posición actual del jugador en el cubo
  int current_position_cache;	// Memoria almacenada de la posición actual del jugador en el cubo
} CubeDat;

/**
 * @brief Variables globales para los hilos.
 */
pthread_t writeLogsThread; // Hilo para escritura en archivo
pthread_mutex_t mutex; // Mutex para sincronizar la escritura

const int writeInterval = 1; // Intervalo de escritura en segundos

/**
 * @brief Escribe periódicamente la posición actual en el archivo "data.dat" con un título.
 *
 * Esta función se encarga de abrir el archivo "log.log" en modo de apertura y escritura,
 * borra cualquier contenido anterior y escribe la posición actual del jugador junto con un título
 * "Coordenadas del jugador" y abajo la direccion de memoria de cube al inicio del archivo si está vacío.
 * Luego, en un bucle infinito, continúa escribiendo la posición actual en el archivo con un intervalo de escritura.
 * El archivo se mantiene abierto durante la ejecución de la aplicación.
 *
 * @param args Un puntero a una estructura CubeDat que contiene la posición actual del jugador.
 * @return NULL (no se utiliza).
 */
void *write_current_position(void *args)
{
    // Convierte los argumentos a un puntero a CubeDat
    CubeDat *dat = (CubeDat *)args;

    // Abre el archivo en modo de apertura y escritura (borra contenido anterior)
    FILE *outfile = fopen("log.log", "w");
    
    // Verifica si se pudo abrir el archivo
    if (!outfile)
    {
        perror("No se pudo abrir el archivo");
        return NULL;

    }

    // Escribe el título si el archivo está vacío
    if (ftell(outfile) == 0)
    {
        fprintf(outfile, "Coordenadas del jugador\n Addr: %p\n\n", (void*)&dat->current_position);
    }

    // Bucle infinito para escribir la posición actual en el archivo
    while (1)
    {
    	// si la ultima escritura de la posicion del jugador es igual a la actual, entra aqui
    	if (dat->current_position_cache == dat->current_position)
    	{
    	   // Nada que hacer
    	}
    	else
    	{
           // Escribe la posición actual en el archivo
           fprintf(outfile, "Posición Actual: %d\n", dat->current_position);
           dat->current_position_cache = dat->current_position;
           fflush(outfile); // Forzar la escritura inmediata al archivo
    	}

        // Duerme durante el intervalo de escritura
        usleep(writeInterval * 1000000); // Esperar en microsegundos
    }

    // Nunca se llega aquí en este bucle infinito
    fclose(outfile); // Asegúrate de cerrar el archivo cuando finaliza la aplicación
    return NULL;
}

/**
 * @brief Inicializa la biblioteca ncurses para configurar una interfaz de usuario en modo texto.
 * Además, desactiva el búfer de línea, oculta el eco de la entrada del usuario y habilita el uso de teclas especiales.
 * Si la terminal admite colores, se inicia el modo de colores y se define un par de colores para texto rojo sobre fondo negro.
 * El par de colores predeterminado se establece como negro sobre negro.
 */
void init_ncurses()
{
  initscr();         	// Inicializar ncurses
  cbreak();          	// Deshabilitar el búfer de línea
  noecho();         	// No mostrar la entrada del usuario
  keypad(stdscr, TRUE); // Habilitar el uso de teclas especiales

  if (has_colors()) // Verificar si la terminal soporta colores
  {
    start_color(); // Iniciar el modo de colores

    // Definir pares de colores (primer par es texto rojo sobre fondo negro)
    init_pair(1, COLOR_RED, COLOR_BLACK);
    
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    // Establecer el par de colores predeterminado (negro sobre negro)
    init_pair(0, COLOR_BLACK, COLOR_BLACK);
    attron(COLOR_PAIR(0)); // Activar el par de colores predeterminado
  }
  refresh();         	// Refrescar la pantalla
}

/**
 * @brief Inicializa el cubo de juego asignando caracteres '#', '@' y '*' de manera aleatoria.
 */
void *writecube(CubeDat *dat)
{
  int i;
  dat->cube = (char *)malloc(dat->row * dat->col);

  // Inicializa la semilla del generador de números aleatorios
  srand(time(NULL));

  // Establece el primer carácter como '#'
  dat->cube[0] = '#';

  for (i = 1; i < dat->row * dat->col - 1; i++) // Comienza desde i = 1
  {
    // Genera un número aleatorio entre 0 y 99
    int random = rand() % 100;
    int random_bonus = rand() % 100;

    // Ajusta el umbral para generar más '#' que '*'
    if (random < 60) // Por ejemplo, 60% de probabilidad de generar '#'
      dat->cube[i] = '#';
    else if (random_bonus < 10)
      dat->cube[i] = '@';
    else
      dat->cube[i] = '*';
  }
}

/**
 * @brief Dibuja una ventana de juego en ncurses y permite la interacción del usuario.
 *
 * @param dat Puntero a la estructura CubeDat que contiene el cubo de juego.
 *            El cubo debe estar previamente inicializado con los datos del juego.
 */
void *draw_cube_window(CubeDat *dat)
{
    int ch;
    int maxRows, maxCols;

    WINDOW *win = stdscr; // Obtener la ventana estándar
    getmaxyx(stdscr, maxRows, maxCols); // Obtener el tamaño de la pantalla
  
    // Crea el hilo para escritura en archivo
    pthread_create(&writeLogsThread, NULL, write_current_position, dat);

    // Inicializa el mutex
    pthread_mutex_init(&mutex, NULL);
    
    while (1)
    {
        clear(); // Limpiar la pantalla
        curs_set(0); // Desactivar cursor

        for (int i = 0; i < dat->row; i++)
        {
	    for (int j = 0; j < dat->col; j++)
	    {
		int index = i * dat->col + j;
		if (index == dat->current_position)
		    attron(A_REVERSE); // Resaltar la posición actual
		    
		if (dat->cube[index] == '#')
		{
		    mvaddch(i, j * 2, 'O');
		}
		else if (dat->cube[index] == '@')
		{
		    attron(COLOR_PAIR(2));
		    mvaddch(i, j * 2, '$');
		    attroff(COLOR_PAIR(1));
		}
		else if (dat->cube[index] == '*')
		{
		    attron(COLOR_PAIR(1)); // Activar el color rojo
		    mvaddch(i, j * 2, 'X');
		    attroff(COLOR_PAIR(1)); // Desactivar el color rojo
		}
		
		if (index == dat->current_position)
		    attroff(A_REVERSE); // Desactivar resaltado
	    }
        }

        refresh(); // Refrescar la pantalla
        ch = getch(); // Obtener la entrada del usuario

        switch (ch)
        {
        case KEY_UP:
            if (dat->current_position >= dat->col)
                dat->current_position -= dat->col;
            break;
        case KEY_DOWN:
            if (dat->current_position + dat->col < dat->row * dat->col)
                dat->current_position += dat->col;
            break;
        case KEY_LEFT:
            if (dat->current_position % dat->col > 0)
                dat->current_position--;
            break;
        case KEY_RIGHT:
            if (dat->current_position % dat->col < dat->col - 1)
                dat->current_position++;
            break;
        case 'q':
        case 'Q':
            // Termina ncurses y sale del programa si el usuario presiona 'q' o 'Q'
            endwin();
            exit(0);
            break;
        }

        // Verifica si la posición actual contiene '*'; si es así, muestra "Game Over" y sale del programa
        if (dat->cube[dat->current_position] == '*')
        {
            pthread_cancel(writeLogsThread); // Cancelar el hilo de escritura
            pthread_mutex_destroy(&mutex); // Destruir el mutex
            clear(); // Limpiar la pantalla
            curs_set(1); // Activar cursor

            attron(COLOR_PAIR(1)); // Activar el par de colores con rojo (debe estar definido previamente)
            mvprintw(maxRows / 2, (maxCols - 9) / 2, "Game Over"); // Imprimir "Game Over" centrado en rojo
            attroff(COLOR_PAIR(1)); // Desactivar el par de colores

            // Mover el cursor a la esquina inferior izquierda
            mvprintw(maxRows - 1, 0, "");

            refresh(); // Refrescar la pantalla
            getch(); // Esperar a que el usuario presione una tecla
            endwin(); // Terminar ncurses
            exit(0);  // Salir del programa
        }
    }
}

/**
 * @brief Función principal del programa.
 */
int main()
{
    CubeDat dat;
    dat.row = 20; // Número de filas en el cubo de juego.
    dat.col = 20; // Número de columnas en el cubo de juego.

    init_ncurses(); // Inicializar la biblioteca ncurses y configurar la interfaz de usuario
    writecube(&dat); // Generar datos aleatorios
    draw_cube_window(&dat); // Dibujar la ventana de juego y permitir la interacción del usuario

    endwin(); // Finalizar ncurses al salir del programa

    return 0;
}
