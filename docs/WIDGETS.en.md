# Widget catalog

[Español](WIDGETS.md)

Select a container, then click or drag a widget from the palette. The inspector shows the properties for the current type. Use **Preview** to try state changes. The identifiers below correspond to the JSON model and help diagnose reports. Some legacy IDs remain readable even when a particular exporter or editor operation has limited support.

The generated `codegen_report.txt` identifies target-specific coverage. Check [the technical reference](REFERENCE.en.md#widget-properties-and-runtime-behavior) for exact limits or [the first-project guide](WORKFLOW.en.md) for a worked example.

## Windows and containers

| Widget | What it does | JSON type |
| --- | --- | --- |
| Root window | Defines a design surface. Set its title, size, position, and window controls. | `window` |
| Child panel | Reserves a region inside another container. Use it for sections with their own size. | `child` |
| Group | Groups related controls so they can move or be reused together. | `group` |
| Tab bar and tab item | Create a bar and add tab items. Place content inside each tab. | `tab_bar`, `tab_item` |
| Collapsible section and tree node | Show or hide children. Use a tree for nested levels and a collapsible section for blocks. | `collapsing_header`, `tree_node` |
| Table | Arranges children in columns. Set the count, headers, and the options shown in the inspector. | `table` |
| Column and row | Arrange children vertically or horizontally in tree order. | `positioner_column`, `positioner_row` |
| Grid and flow | Grid uses columns and spacing. Flow wraps children when the remaining row width is insufficient. | `positioner_grid`, `positioner_flow` |
| Loader and component | Loader shows children according to its loaded state. Component groups children under a template name. | `loader`, `component` |
| Repeater | Draws copies of a template. Set the count and spacing. Edit the template in design mode. | `repeater` |
| Animation | Applies an effect to its children. Set duration, easing, loop, and initial playback. | `animation` |

## Text, actions, and spacing

| Widget | What it does | JSON type |
| --- | --- | --- |
| Text, colored text, wrapped text, and bullet text | Display content. Choose a color, wrapping, or bullet style for the presentation. | `text`, `text_colored`, `text_wrapped`, `bullet_text` |
| Help marker | Shows help when the pointer hovers over the marker. | `help_marker` |
| Button, small button, arrow button, and color button | Provide action controls with different shapes. Connect application logic in the generated host. | `button`, `small_button`, `arrow_button`, `color_button` |
| Title bar button | Place it directly under a window. The host can read whether it was activated in the current frame. | `title_bar_button` |
| Menu bar | Set menu placement and items in a window. The host supplies the application actions. | `menu_bar` |
| Separator and separator with text | Visually divides groups of controls. The text variant labels the section. | `separator`, `separator_text` |
| Spacing and fixed space | Add space to the layout. Use a custom size to reserve a specific region. | `spacing`, `dummy` |
| SameLine | Places the next item on the same row. You can also set the widget's SameLine property. | `same_line` |

## Input and selection

| Widget | What it does | JSON type |
| --- | --- | --- |
| Checkbox and toggle | Represent an on/off value. Set the initial value in the inspector. | `checkbox`, `toggle` |
| Radio and selectable | Radio buttons with the same variable share a selection. A selectable presents a single item that can be selected. | `radio`, `selectable` |
| Single-line and multiline text input | Set initial text and a placeholder where supported. Try editing it in Preview. | `input_text`, `input_multiline` |
| Integer and decimal input | Accept typed numeric values. Use a format appropriate for the value type. | `input_int`, `input_float` |
| Sliders and angle | Constrain values to a range. Vector variants expose multiple components, and the angle control edits an orientation. | `slider_int`, `slider_float`, `slider_float2`, `slider_float3`, `slider_angle` |
| Numeric drag controls | Change numeric values by dragging. Set speed, format, and bounds as supported. | `drag_int`, `drag_float`, `drag_float3` |
| RGB and RGBA color editors | Edit an application color value. RGBA includes an opacity channel. | `color_edit3`, `color_edit4` |
| Combo and list box | Select from options defined in the inspector. A combo saves space, while a list box keeps options visible. | `combo`, `listbox` |
| List, grid, and path views | Display a model of items. Set the initial index and try selection in Preview. | `list_view`, `grid_view`, `path_view` |
| Knob | Adjusts a value with a rotary control. Enable it in the catalog before inserting it. | `knob` |

## Images, charts, and status

| Widget | What it does | JSON type |
| --- | --- | --- |
| Image and image button | Import a local PNG or JPEG through the inspector. The button supports activation, while the plain image displays the asset. | `image`, `image_button` |
| Line plot and histogram | Plot simple numeric series. Adjust values and scales in the inspector. | `plot_lines`, `plot_histogram` |
| 2D chart | Choose line, area, bar, doughnut, scatter, or smooth curve. Use **Edit values** to paste a series. | `chart_2d` |
| 3D chart | Choose bars, scatter, or surface. Adjust elevation and azimuth. Rotate temporarily with the mouse in Preview. | `chart_3d` |
| Linear and circular progress | Use a fraction from 0 to 1. Set overlay or center text as needed. | `progress_bar`, `circular_progress` |
| Loading spinner | Represents work in progress. Set the speed, with zero stopping the animation. | `spinner` |
