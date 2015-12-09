# Servidor de Streaming de Subtítulos

El servidor transmite los subtitulos de un archivo .srt pasado como argumento. Los clientes se conectan y desconectan en
cualquier momento al streaming y reciben los subtitulos que se transmiten respetando los tiempos de aparición y permanencia.

La obtención y transmisión de los datos se realiza por un hilo exclusivo para la tarea, mientras que un proceso hijo se encarga de controlar 
controlar los tiempos de timeout de las conexiones clientes, mientras el proceso principal se encarga de aceptar nuevas conexiones y 
cerrar conexiones terminadas, indicando esta información por consola.
