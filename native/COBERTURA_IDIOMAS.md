# Cobertura de idiomas

Los catálogos incluidos en el editor nativo contienen las mismas claves
en español, inglés, francés, coreano, japonés y chino simplificado.
Los catálogos incluyen las traducciones de los controles nativos.

| Idioma | Claves del catálogo sin traducción |
| --- | --- |
| Español | 0 |
| Inglés | 0 |
| Francés | 0 |
| Coreano | 0 |
| Japonés | 0 |
| Chino simplificado | 0 |

Quedan cadenas fuera del catálogo en los seis idiomas.

- Opciones especializadas de algunos widgets, como variantes gráficas, iconos y perfiles visuales.
- Mensajes internos de exportadores, dependencias, historial y servicios del asistente. Los estados de compilación y la detección de herramientas ya tienen traducción en su interfaz.
- Textos del compilador, del sistema operativo o de un proveedor externo. El editor muestra su salida original.
- El catálogo de widgets y la biblioteca personal se localizarán al integrarlos en el editor nativo.

Los documentos, el código, los nombres de widgets del proyecto y las
conversaciones conservan el texto de sus autores. La cobertura tipográfica
depende de las fuentes instaladas. La prueba de entrada introduce texto
UTF-8 en Dear ImGui y la prueba de glifos comprueba los seis catálogos con
el atlas del editor. La composición con el IME del sistema queda por validar.
