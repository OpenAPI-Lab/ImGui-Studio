# Seguridad y reportes

[English](SECURITY.en.md)


Si un fallo puede exponer datos o ejecutar acciones inesperadas, usa el canal privado de GitHub si está habilitado. Si no encuentras uno, pide al mantenedor un canal privado antes de compartir detalles. Incluye versión, plataforma, impacto, pasos mínimos y registros redactados. Los errores generales se reportan mediante [el flujo de contribuciones](CONTRIBUTING.md).

## Datos y ejecución

- Los JSON de proyecto y de widget pueden incluir imágenes embebidas. Revisa su contenido antes de adjuntarlos. Mantén credenciales, historial y datos privados fuera de informes públicos.
- Run / Build ejecuta herramientas y, al pulsar Run, la aplicación que generan. Los SDK y rutas seleccionados pasan a la cadena de compilación con los permisos de la cuenta que ejecuta el editor.
- Al enviar una consulta, el asistente transmite la consulta, contexto reciente y un resumen del diseño al proveedor elegido. Sus propuestas pasan por revisión y se aplican mediante una acción explícita. Las claves se manejan por separado de los proyectos.
- El catálogo comprueba los archivos fijados de imgui-knobs. La vista previa usa el adaptador integrado en la compilación.

Consulta los [contratos del proyecto](docs/PROJECT_FORMAT.md) y la [referencia técnica](docs/REFERENCE.md) para conocer la persistencia y el alcance de las operaciones.
