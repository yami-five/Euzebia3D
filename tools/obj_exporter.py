from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_OUTPUT_PATH = SCRIPT_DIR / "obj_converted.txt"


@dataclass
class ObjData:
    vertices: list[str]
    faces: list[int]
    texture_coords: list[str]
    uv: list[int]
    vn: list[str]
    normals: list[int]


def to_c_identifier(value: str) -> str:
    identifier = re.sub(r"[^0-9A-Za-z_]", "_", value.strip())
    if not identifier:
        return "model"
    if identifier[0].isdigit():
        return f"_{identifier}"
    return identifier


def _parse_obj_index(raw: str, count: int) -> int:
    index = int(raw)
    if index == 0:
        raise ValueError("OBJ index cannot be 0.")
    return index - 1 if index > 0 else count + index


def _append_face_vertex(
    token: str,
    vertex_count: int,
    vt_count: int,
    vn_count: int,
    faces: list[int],
    uv: list[int],
    normals: list[int],
) -> None:
    parts = token.split("/")
    if len(parts) < 3 or not parts[1] or not parts[2]:
        raise ValueError(
            "Face entry must contain v/vt/vn. Found token: "
            f"'{token}'. Exporter expects UV and normal indices."
        )

    vertex_index = _parse_obj_index(parts[0], vertex_count)
    uv_index = _parse_obj_index(parts[1], vt_count)
    normal_index = _parse_obj_index(parts[2], vn_count)

    if vertex_index < 0 or vertex_index >= vertex_count:
        raise ValueError(f"Vertex index out of range in face token '{token}'.")
    if uv_index < 0 or uv_index >= vt_count:
        raise ValueError(f"UV index out of range in face token '{token}'.")
    if normal_index < 0 or normal_index >= vn_count:
        raise ValueError(f"Normal index out of range in face token '{token}'.")

    faces.append(vertex_index)
    uv.append(uv_index)
    normals.append(normal_index)


def parse_obj_file(input_path: Path) -> ObjData:
    vertices: list[str] = []
    faces: list[int] = []
    texture_coords: list[str] = []
    uv: list[int] = []
    vn: list[str] = []
    normals: list[int] = []

    raw_vertices_count = 0
    raw_vt_count = 0
    raw_vn_count = 0

    with input_path.open(encoding="utf-8", errors="ignore") as obj_file:
        for line_number, raw_line in enumerate(obj_file, start=1):
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            if line.startswith("v "):
                parts = line.split()
                if len(parts) < 4:
                    raise ValueError(f"Invalid vertex at line {line_number}: '{raw_line.rstrip()}'.")
                vertices.append(f"{round(float(parts[1]), 6)}f")
                vertices.append(f"{round(float(parts[2]), 6)}f")
                vertices.append(f"{round(float(parts[3]), 6)}f")
                raw_vertices_count += 1
                continue

            if line.startswith("vt "):
                parts = line.split()
                if len(parts) < 3:
                    raise ValueError(
                        f"Invalid texture coordinate at line {line_number}: '{raw_line.rstrip()}'."
                    )
                texture_coords.append(f"{round(float(parts[1]), 6)}f")
                texture_coords.append(f"{round(float(parts[2]), 6)}f")
                raw_vt_count += 1
                continue

            if line.startswith("vn "):
                parts = line.split()
                if len(parts) < 4:
                    raise ValueError(f"Invalid normal at line {line_number}: '{raw_line.rstrip()}'.")
                vn.append(f"{round(float(parts[1]), 6)}f")
                vn.append(f"{round(float(parts[2]), 6)}f")
                vn.append(f"{round(float(parts[3]), 6)}f")
                raw_vn_count += 1
                continue

            if line.startswith("f "):
                parts = line.split()[1:]
                if len(parts) < 3:
                    raise ValueError(f"Invalid face at line {line_number}: '{raw_line.rstrip()}'.")

                # Fan triangulation for polygons with >3 vertices.
                for i in range(1, len(parts) - 1):
                    tri = (parts[0], parts[i], parts[i + 1])
                    for token in tri:
                        _append_face_vertex(
                            token=token,
                            vertex_count=raw_vertices_count,
                            vt_count=raw_vt_count,
                            vn_count=raw_vn_count,
                            faces=faces,
                            uv=uv,
                            normals=normals,
                        )

    return ObjData(
        vertices=vertices,
        faces=faces,
        texture_coords=texture_coords,
        uv=uv,
        vn=vn,
        normals=normals,
    )


def build_output_text(model_name: str, obj_data: ObjData) -> str:
    vertices_text = ",".join(obj_data.vertices)
    faces_text = ",".join(str(value) for value in obj_data.faces)
    texture_coords_text = ",".join(obj_data.texture_coords)
    uv_text = ",".join(str(value) for value in obj_data.uv)
    vn_text = ",".join(obj_data.vn)
    normals_text = ",".join(str(value) for value in obj_data.normals)

    lines = [
        model_name,
        f"const float {model_name}Vertices[{len(obj_data.vertices)}]={{{vertices_text}}};",
        f"const uint16_t {model_name}Faces[{len(obj_data.faces)}] = {{{faces_text}}};",
        f"const float {model_name}TextureCoords[{len(obj_data.texture_coords)}] = {{{texture_coords_text}}};",
        f"const uint16_t {model_name}UV[{len(obj_data.faces)}] = {{{uv_text}}};",
        f"const float {model_name}VN[{len(obj_data.vn)}] = {{{vn_text}}};",
        f"const uint16_t {model_name}Normals[{len(obj_data.faces)}] = {{{normals_text}}};",
        "",
        "{",
        f"    .vertices = {model_name}Vertices,",
        f"    .faces = {model_name}Faces,",
        f"    .textureCoords = {model_name}TextureCoords,",
        f"    .uv = {model_name}UV,",
        f"    .normals = {model_name}Normals,",
        f"    .vn = {model_name}VN,",
        f"    .verticesCounter = {len(obj_data.vertices) // 3},",
        f"    .facesCounter = {len(obj_data.faces) // 3},",
        f"    .textureCoordsCounter = {len(obj_data.texture_coords) // 2},",
        f"    .vnCounter = {len(obj_data.vn) // 3},",
        "},",
    ]
    return "\n".join(lines)


def export_obj(input_path: Path, model_name: str, output_path: Path) -> tuple[ObjData, str]:
    obj_data = parse_obj_file(input_path)
    output_text = build_output_text(model_name=model_name, obj_data=obj_data)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(output_text, encoding="utf-8")
    return obj_data, output_text


def run_gui() -> int:
    try:
        from PySide6.QtWidgets import (
            QApplication,
            QFileDialog,
            QFormLayout,
            QHBoxLayout,
            QLabel,
            QLineEdit,
            QMainWindow,
            QMessageBox,
            QPlainTextEdit,
            QPushButton,
            QVBoxLayout,
            QWidget,
        )
    except ImportError:
        print("PySide6 is not installed. Install it with: pip install PySide6", file=sys.stderr)
        return 1

    class ObjExporterWindow(QMainWindow):
        def __init__(self) -> None:
            super().__init__()
            self.setWindowTitle("OBJ Exporter")
            self.resize(1100, 720)
            self.last_output = ""

            central_widget = QWidget()
            self.setCentralWidget(central_widget)
            root_layout = QVBoxLayout(central_widget)

            form_layout = QFormLayout()
            root_layout.addLayout(form_layout)

            self.input_edit = QLineEdit()
            self.input_browse_button = QPushButton("Browse...")
            input_row = QHBoxLayout()
            input_row.addWidget(self.input_edit)
            input_row.addWidget(self.input_browse_button)
            form_layout.addRow("OBJ file:", input_row)

            self.name_edit = QLineEdit()
            form_layout.addRow("Model name:", self.name_edit)

            self.output_edit = QLineEdit(str(DEFAULT_OUTPUT_PATH))
            self.output_browse_button = QPushButton("Browse...")
            output_row = QHBoxLayout()
            output_row.addWidget(self.output_edit)
            output_row.addWidget(self.output_browse_button)
            form_layout.addRow("Output file:", output_row)

            actions_row = QHBoxLayout()
            self.export_button = QPushButton("Export and save")
            self.copy_button = QPushButton("Copy output")
            self.copy_button.setEnabled(False)
            actions_row.addWidget(self.export_button)
            actions_row.addWidget(self.copy_button)
            root_layout.addLayout(actions_row)

            self.status_label = QLabel("Select an OBJ file and click Export and save.")
            root_layout.addWidget(self.status_label)

            self.summary_label = QLabel("Summary: -")
            root_layout.addWidget(self.summary_label)

            self.preview_edit = QPlainTextEdit()
            self.preview_edit.setReadOnly(True)
            root_layout.addWidget(self.preview_edit)

            self.input_browse_button.clicked.connect(self.select_input_file)
            self.output_browse_button.clicked.connect(self.select_output_file)
            self.export_button.clicked.connect(self.export_file)
            self.copy_button.clicked.connect(self.copy_output)

        def select_input_file(self) -> None:
            default_dir = SCRIPT_DIR.parent / "assets"
            file_path, _ = QFileDialog.getOpenFileName(
                self,
                "Select OBJ file",
                str(default_dir),
                "OBJ files (*.obj);;All files (*.*)",
            )
            if not file_path:
                return

            input_path = Path(file_path)
            self.input_edit.setText(str(input_path))
            if not self.name_edit.text().strip():
                self.name_edit.setText(to_c_identifier(input_path.stem))

        def select_output_file(self) -> None:
            default_output = self.output_edit.text().strip() or str(DEFAULT_OUTPUT_PATH)
            file_path, _ = QFileDialog.getSaveFileName(
                self,
                "Select output file",
                default_output,
                "Text files (*.txt);;C Header files (*.h);;All files (*.*)",
            )
            if file_path:
                self.output_edit.setText(file_path)

        def export_file(self) -> None:
            input_text = self.input_edit.text().strip()
            output_text = self.output_edit.text().strip()
            if not input_text:
                QMessageBox.warning(self, "Missing input", "Choose an OBJ file first.")
                return
            if not output_text:
                QMessageBox.warning(self, "Missing output", "Set output file path.")
                return

            input_path = Path(input_text)
            if not input_path.exists():
                QMessageBox.warning(self, "Missing input file", f"File not found: {input_path}")
                return

            model_name = to_c_identifier(self.name_edit.text() or input_path.stem)
            self.name_edit.setText(model_name)
            output_path = Path(output_text)

            try:
                obj_data, exported_text = export_obj(
                    input_path=input_path,
                    model_name=model_name,
                    output_path=output_path,
                )
            except Exception as exc:
                QMessageBox.critical(self, "Export failed", str(exc))
                return

            self.last_output = exported_text
            self.preview_edit.setPlainText(exported_text)
            self.copy_button.setEnabled(True)
            self.status_label.setText(f"Saved output to {output_path}")
            self.summary_label.setText(
                "Summary: "
                f"vertices={len(obj_data.vertices)//3}, "
                f"faces={len(obj_data.faces)//3}, "
                f"vtn={len(obj_data.texture_coords)//2}, "
                f"nn={len(obj_data.vn)//3}"
            )

        def copy_output(self) -> None:
            if not self.last_output:
                return
            QApplication.clipboard().setText(self.last_output)
            self.status_label.setText("Output copied to clipboard.")

    app = QApplication(sys.argv)
    window = ObjExporterWindow()
    window.show()
    return app.exec()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export OBJ geometry to C arrays used by the renderer."
    )
    parser.add_argument(
        "-i",
        "--input",
        type=Path,
        help="Path to input OBJ file. If omitted, the PySide6 UI is started.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_PATH,
        help=f"Path to output text file (default: {DEFAULT_OUTPUT_PATH}).",
    )
    parser.add_argument(
        "-n",
        "--name",
        type=str,
        help="Model name used in generated C identifiers (default: input file name).",
    )
    parser.add_argument(
        "--stdout",
        action="store_true",
        help="Print generated output to stdout.",
    )
    parser.add_argument(
        "--no-ui",
        action="store_true",
        help="Run in command-line mode only.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.input is None and not args.no_ui:
        return run_gui()

    if args.input is None:
        print("Missing --input in --no-ui mode.", file=sys.stderr)
        return 2

    input_path = args.input
    if not input_path.exists():
        print(f"Input file not found: {input_path}", file=sys.stderr)
        return 2

    model_name = to_c_identifier(args.name or input_path.stem)
    try:
        obj_data, output_text = export_obj(
            input_path=input_path,
            model_name=model_name,
            output_path=args.output,
        )
    except Exception as exc:
        print(f"Export failed: {exc}", file=sys.stderr)
        return 1

    print(f"Model: {model_name}")
    print(f"Output: {args.output}")
    print(
        "Summary: "
        f"vertices={len(obj_data.vertices)//3}, "
        f"faces={len(obj_data.faces)//3}, "
        f"vtn={len(obj_data.texture_coords)//2}, "
        f"nn={len(obj_data.vn)//3}"
    )

    if args.stdout:
        print(output_text)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
