from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from PIL import Image

SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_OUTPUT_PATH = SCRIPT_DIR / "img_converted.txt"


def rgb_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def to_c_identifier(value: str) -> str:
    identifier = re.sub(r"[^0-9A-Za-z_]", "_", value.strip())
    if not identifier:
        return "texture"
    if identifier[0].isdigit():
        return f"_{identifier}"
    return identifier


def convert_image_to_rgb565(image_path: Path, reverse_x: bool = True) -> tuple[int, int, list[int]]:
    with Image.open(image_path) as image:
        rgb_image = image.convert("RGB")
        img_x, img_y = rgb_image.size

        converted_pixels: list[int] = []
        for y in range(img_y):
            x_range = range(img_x - 1, -1, -1) if reverse_x else range(img_x)
            for x in x_range:
                r, g, b = rgb_image.getpixel((x, y))
                converted_pixels.append(rgb_to_rgb565(r, g, b))

    return img_x, img_y, converted_pixels


def format_c_array(array_name: str, pixels: list[int]) -> str:
    pixels_str = ",".join(str(value) for value in pixels)
    return f"static const uint16_t {array_name}[{len(pixels)}]={{{pixels_str}}};"


def convert_and_save(
    input_path: Path,
    array_name: str,
    output_path: Path,
    reverse_x: bool = True,
) -> tuple[int, int, str]:
    img_x, img_y, pixels = convert_image_to_rgb565(input_path, reverse_x=reverse_x)
    c_code = format_c_array(array_name=array_name, pixels=pixels)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(c_code, encoding="utf-8")
    return img_x, img_y, c_code


def run_gui() -> int:
    try:
        from PySide6.QtCore import Qt
        from PySide6.QtGui import QImage, QPixmap
        from PySide6.QtWidgets import (
            QApplication,
            QCheckBox,
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
        print(
            "PySide6 is not installed. Install it with: pip install PySide6",
            file=sys.stderr,
        )
        return 1

    class TextureConverterWindow(QMainWindow):
        def __init__(self) -> None:
            super().__init__()
            self.setWindowTitle("Texture Converter")
            self.resize(1100, 700)
            self.last_output = ""
            self.source_pixmap: QPixmap | None = None
            self.source_image_bytes: bytes | None = None

            central_widget = QWidget()
            self.setCentralWidget(central_widget)
            root_layout = QVBoxLayout(central_widget)

            form_layout = QFormLayout()
            root_layout.addLayout(form_layout)

            self.input_edit = QLineEdit()
            self.input_browse_btn = QPushButton("Browse...")
            input_layout = QHBoxLayout()
            input_layout.addWidget(self.input_edit)
            input_layout.addWidget(self.input_browse_btn)
            form_layout.addRow("BMP file:", input_layout)

            self.name_edit = QLineEdit()
            form_layout.addRow("Array name:", self.name_edit)

            self.output_edit = QLineEdit(str(DEFAULT_OUTPUT_PATH))
            self.output_browse_btn = QPushButton("Browse...")
            output_layout = QHBoxLayout()
            output_layout.addWidget(self.output_edit)
            output_layout.addWidget(self.output_browse_btn)
            form_layout.addRow("Output file:", output_layout)

            self.reverse_x_checkbox = QCheckBox(
                "Read each row from right to left (same as current texture_converter behavior)"
            )
            self.reverse_x_checkbox.setChecked(True)
            root_layout.addWidget(self.reverse_x_checkbox)

            actions_layout = QHBoxLayout()
            self.convert_btn = QPushButton("Convert and save")
            self.copy_btn = QPushButton("Copy generated code")
            self.copy_btn.setEnabled(False)
            actions_layout.addWidget(self.convert_btn)
            actions_layout.addWidget(self.copy_btn)
            root_layout.addLayout(actions_layout)

            self.status_label = QLabel("Select a BMP file and click Convert and save.")
            root_layout.addWidget(self.status_label)

            self.image_info_label = QLabel("Image preview: no file selected.")
            root_layout.addWidget(self.image_info_label)

            self.image_preview_label = QLabel("Image preview")
            self.image_preview_label.setMinimumHeight(220)
            self.image_preview_label.setAlignment(Qt.AlignCenter)
            self.image_preview_label.setStyleSheet(
                "QLabel { border: 1px solid #6f6f6f; background-color: #151515; color: #e0e0e0; }"
            )
            root_layout.addWidget(self.image_preview_label)

            self.preview_edit = QPlainTextEdit()
            self.preview_edit.setReadOnly(True)
            root_layout.addWidget(self.preview_edit)

            self.input_browse_btn.clicked.connect(self.select_input_file)
            self.input_edit.editingFinished.connect(self.try_preview_from_input_path)
            self.output_browse_btn.clicked.connect(self.select_output_file)
            self.convert_btn.clicked.connect(self.convert_file)
            self.copy_btn.clicked.connect(self.copy_output)

        def select_input_file(self) -> None:
            default_dir = SCRIPT_DIR.parent / "assets"
            selected_file, _ = QFileDialog.getOpenFileName(
                self,
                "Select BMP file",
                str(default_dir),
                "Bitmap files (*.bmp);;All files (*.*)",
            )
            if not selected_file:
                return

            input_path = Path(selected_file)
            self.input_edit.setText(str(input_path))
            self.name_edit.setText(to_c_identifier(input_path.stem))
            self.update_image_preview(input_path)

        def try_preview_from_input_path(self) -> None:
            input_value = self.input_edit.text().strip()
            if not input_value:
                self.clear_image_preview("Image preview: no file selected.")
                return

            input_path = Path(input_value)
            if input_path.exists() and input_path.is_file():
                self.name_edit.setText(to_c_identifier(input_path.stem))
                self.update_image_preview(input_path)
                return

            self.clear_image_preview("Image preview: file not found.")

        def select_output_file(self) -> None:
            default_output = self.output_edit.text().strip() or str(DEFAULT_OUTPUT_PATH)
            selected_file, _ = QFileDialog.getSaveFileName(
                self,
                "Select output file",
                default_output,
                "Text files (*.txt);;C Header files (*.h);;All files (*.*)",
            )
            if selected_file:
                self.output_edit.setText(selected_file)

        def convert_file(self) -> None:
            input_value = self.input_edit.text().strip()
            output_value = self.output_edit.text().strip()
            if not input_value:
                QMessageBox.warning(self, "Missing input", "Choose a BMP file first.")
                return
            if not output_value:
                QMessageBox.warning(self, "Missing output", "Set output file path.")
                return

            input_path = Path(input_value)
            if not input_path.exists():
                QMessageBox.warning(self, "Missing input file", f"File not found: {input_path}")
                return
            self.update_image_preview(input_path)

            array_name = to_c_identifier(self.name_edit.text() or input_path.stem)
            self.name_edit.setText(array_name)
            output_path = Path(output_value)

            try:
                img_x, img_y, c_code = convert_and_save(
                    input_path=input_path,
                    array_name=array_name,
                    output_path=output_path,
                    reverse_x=self.reverse_x_checkbox.isChecked(),
                )
            except Exception as exc:
                QMessageBox.critical(self, "Conversion failed", str(exc))
                return

            self.last_output = c_code
            self.preview_edit.setPlainText(c_code)
            self.copy_btn.setEnabled(True)
            self.status_label.setText(
                f"Saved {img_x}x{img_y} ({img_x * img_y} pixels) to {output_path}"
            )

        def copy_output(self) -> None:
            if not self.last_output:
                return

            QApplication.clipboard().setText(self.last_output)
            self.status_label.setText("Generated code copied to clipboard.")

        def update_image_preview(self, image_path: Path) -> None:
            pixmap = self.load_preview_pixmap(image_path)
            if pixmap.isNull():
                self.clear_image_preview(f"Image preview: cannot load {image_path.name}.")
                return

            self.source_pixmap = pixmap
            self.image_info_label.setText(
                f"Image preview: {image_path.name} ({pixmap.width()}x{pixmap.height()} px)"
            )
            self.refresh_image_preview()

        def refresh_image_preview(self) -> None:
            if self.source_pixmap is None or self.source_pixmap.isNull():
                return

            target_size = self.image_preview_label.size()
            if target_size.width() <= 0 or target_size.height() <= 0:
                return

            scaled = self.source_pixmap.scaled(
                target_size,
                Qt.KeepAspectRatio,
                Qt.SmoothTransformation,
            )
            self.image_preview_label.setPixmap(scaled)

        def clear_image_preview(self, info_text: str) -> None:
            self.source_pixmap = None
            self.source_image_bytes = None
            self.image_info_label.setText(info_text)
            self.image_preview_label.clear()
            self.image_preview_label.setText("Image preview")

        def load_preview_pixmap(self, image_path: Path) -> QPixmap:
            # Pillow-based loading keeps preview reliable for BMP variants.
            try:
                with Image.open(image_path) as image:
                    rgb_image = image.convert("RGB")
                    width, height = rgb_image.size
                    self.source_image_bytes = rgb_image.tobytes("raw", "RGB")
            except Exception:
                return QPixmap()

            bytes_per_line = width * 3
            q_image = QImage(
                self.source_image_bytes,
                width,
                height,
                bytes_per_line,
                QImage.Format_RGB888,
            )
            return QPixmap.fromImage(q_image)

        def resizeEvent(self, event) -> None:
            super().resizeEvent(event)
            self.refresh_image_preview()

    app = QApplication(sys.argv)
    window = TextureConverterWindow()
    window.show()
    return app.exec()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert BMP texture to a C uint16_t array in RGB565 format."
    )
    parser.add_argument(
        "-i",
        "--input",
        type=Path,
        help="Path to input BMP file. If omitted, the PySide6 UI is started.",
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
        help="Array name in generated C code (default: input file name).",
    )
    parser.add_argument(
        "--left-to-right",
        action="store_true",
        help="Read image rows from left to right. Default keeps current right-to-left behavior.",
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

    output_path = args.output
    array_name = to_c_identifier(args.name or input_path.stem)

    try:
        img_x, img_y, _ = convert_and_save(
            input_path=input_path,
            array_name=array_name,
            output_path=output_path,
            reverse_x=not args.left_to_right,
        )
    except Exception as exc:
        print(f"Conversion failed: {exc}", file=sys.stderr)
        return 1

    pixel_count = img_x * img_y
    print(f"Converted image: {img_x}x{img_y} ({pixel_count} pixels)")
    print(f"Output: {output_path}")
    print(f"Array name: {array_name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
