# Widget library and catalog

[Español](WIDGET_LIBRARY.md)


## Custom definitions

1. Open **Widgets → My library → Create widget**. Enter a name and choose **Blank canvas** or a control as the **Starting point**. You can also start from the current selection when that control is supported.
2. Choose **Open designer**. Add controls and test the result in preview. Save to **My library** to return to the project.
3. Select a container in your project. In **Custom widgets**, select your definition and choose **Insert**. To share it, choose **Export widget** and enter a new path.

In **Widgets → My library**, create a definition or select **Add from file**. Import the [process card](../examples/tarjeta-de-proceso.widget.json), insert it twice, and use the inspector to change one instance. Each copy's IDs and properties are saved in the project. Editing a definition affects future insertions.

The importer validates a definition before adding it to the library. Limits in code are 256 entries, 120 bytes for a name, 1,024 nodes per widget, and 16 MiB per definition file. Embedded images travel with the tree. See the [library contract](../include/imguistudio/widgets/studio_widget_library.hpp) when changing import or persistence.

## Catalog knob

Open **Widgets → My library → Catalog** and choose **Install** on imgui-knobs. Once installation completes, insert a **Knob** from **Installed extensions**. Set its range and initial value in the inspector and try it in **Preview**. [Widget catalog](WIDGETS.en.md)

The **Catalog** installs the pinned imgui-knobs revision. The client fetches header, implementation, and license and compares each file with reviewed sources. It writes `extensions/imgui-knobs.json` after validating the download and acquiring a session lock. This enables the knob compiled into the editor. C++ exports include sources and the MIT notice. Targets without knob rendering emit a placeholder and a diagnostic.

Removing the extension hides its palette entry. Nodes already in a project retain their data. Reinstall the extension to restore preview.

## Contribute an extension

Document its origin, revision, license, widget, and affected targets. Review the native adapter, file verification, and export support together. Include a small project and expected diagnostics for partial targets. Use `NativeExtensionCatalogTest` for installation, rejection of changed content, and C++ export.

[Catalog implementation](../src/services/studio_extensions.cpp) · [imgui-knobs revision](../third_party/imgui-knobs/SOURCE.md)
