from __future__ import annotations

import argparse
import sys
from pathlib import Path

from PIL import Image

SCRIPT_DIR = Path(__file__).resolve().parent
ASSETS_DIR = SCRIPT_DIR.parent / "assets"
DEFAULT_INPUT_PATH = ASSETS_DIR / "letters.bmp"
DEFAULT_OUTPUT_PATH = ASSETS_DIR / "font_converted.txt"
DEFAULT_GLYPH_WIDTH = 16
DEFAULT_GLYPH_HEIGHT = 16


def rgb_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def transform_font_atlas(
    image: Image.Image,
    glyph_width: int = DEFAULT_GLYPH_WIDTH,
    glyph_height: int = DEFAULT_GLYPH_HEIGHT,
    transpose_glyphs: bool = False,
) -> Image.Image:
    if glyph_width <= 0 or glyph_height <= 0:
        raise ValueError("Glyph width and height must be greater than 0.")
    if transpose_glyphs and glyph_width != glyph_height:
        raise ValueError("Transposed font export requires square glyphs.")

    rgb_image = image.convert("RGB")
    img_x, img_y = rgb_image.size
    if img_x % glyph_width != 0 or img_y % glyph_height != 0:
        raise ValueError(
            "Image dimensions must be divisible by glyph size. "
            f"Image: {img_x}x{img_y}, glyph: {glyph_width}x{glyph_height}."
        )

    if not transpose_glyphs:
        return rgb_image

    converted_image = Image.new("RGB", rgb_image.size)
    for y in range(0, img_y, glyph_height):
        for x in range(0, img_x, glyph_width):
            sprite = rgb_image.crop((x, y, x + glyph_width, y + glyph_height))
            converted_sprite = sprite.rotate(90).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
            converted_image.paste(converted_sprite, (x, y))

    return converted_image


def convert_font_to_rgb565(
    input_path: Path,
    glyph_width: int = DEFAULT_GLYPH_WIDTH,
    glyph_height: int = DEFAULT_GLYPH_HEIGHT,
    transpose_glyphs: bool = False,
) -> tuple[int, int, int, list[int]]:
    with Image.open(input_path) as image:
        converted_image = transform_font_atlas(
            image=image,
            glyph_width=glyph_width,
            glyph_height=glyph_height,
            transpose_glyphs=transpose_glyphs,
        )
        img_x, img_y = converted_image.size
        glyph_columns = img_x // glyph_width
        glyph_rows = img_y // glyph_height
        glyph_count = glyph_columns * glyph_rows
        strip_width = glyph_count * glyph_width

        converted_pixels: list[int] = []
        for glyph_y in range(glyph_height):
            for glyph_index in range(glyph_count):
                source_glyph_x = (glyph_index % glyph_columns) * glyph_width
                source_glyph_y = (glyph_index // glyph_columns) * glyph_height
                for glyph_x in range(glyph_width):
                    r, g, b = converted_image.getpixel(
                        (source_glyph_x + glyph_x, source_glyph_y + glyph_y)
                    )
                    converted_pixels.append(rgb_to_rgb565(r, g, b))

    return strip_width, glyph_height, glyph_count, converted_pixels


def format_pixels(pixels: list[int]) -> str:
    return ",".join(str(value) for value in pixels)


def convert_and_save(
    input_path: Path,
    output_path: Path,
    glyph_width: int = DEFAULT_GLYPH_WIDTH,
    glyph_height: int = DEFAULT_GLYPH_HEIGHT,
    transpose_glyphs: bool = False,
) -> tuple[int, int, int, str]:
    img_x, img_y, glyph_count, pixels = convert_font_to_rgb565(
        input_path=input_path,
        glyph_width=glyph_width,
        glyph_height=glyph_height,
        transpose_glyphs=transpose_glyphs,
    )
    output_text = format_pixels(pixels)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(output_text, encoding="utf-8")
    return img_x, img_y, glyph_count, output_text


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
            QSpinBox,
            QVBoxLayout,
            QWidget,
        )
    except ImportError:
        print("PySide6 is not installed. Install it with: pip install PySide6", file=sys.stderr)
        return 1

    class FontExporterWindow(QMainWindow):
        def __init__(self) -> None:
            super().__init__()
            self.setWindowTitle("Font Exporter")
            self.resize(1100, 720)
            self.last_output = ""
            self.source_pixmap: QPixmap | None = None
            self.source_image_bytes: bytes | None = None

            central_widget = QWidget()
            self.setCentralWidget(central_widget)
            root_layout = QVBoxLayout(central_widget)

            form_layout = QFormLayout()
            root_layout.addLayout(form_layout)

            self.input_edit = QLineEdit(str(DEFAULT_INPUT_PATH))
            self.input_browse_button = QPushButton("Browse...")
            input_row = QHBoxLayout()
            input_row.addWidget(self.input_edit)
            input_row.addWidget(self.input_browse_button)
            form_layout.addRow("Font BMP file:", input_row)

            glyph_size_row = QHBoxLayout()
            self.glyph_width_spin = QSpinBox()
            self.glyph_width_spin.setRange(1, 1024)
            self.glyph_width_spin.setValue(DEFAULT_GLYPH_WIDTH)
            self.glyph_height_spin = QSpinBox()
            self.glyph_height_spin.setRange(1, 1024)
            self.glyph_height_spin.setValue(DEFAULT_GLYPH_HEIGHT)
            glyph_size_row.addWidget(self.glyph_width_spin)
            glyph_size_row.addWidget(QLabel("x"))
            glyph_size_row.addWidget(self.glyph_height_spin)
            form_layout.addRow("Glyph size:", glyph_size_row)

            self.output_edit = QLineEdit(str(DEFAULT_OUTPUT_PATH))
            self.output_browse_button = QPushButton("Browse...")
            output_row = QHBoxLayout()
            output_row.addWidget(self.output_edit)
            output_row.addWidget(self.output_browse_button)
            form_layout.addRow("Output file:", output_row)

            self.transpose_checkbox = QCheckBox("Transpose each glyph before export")
            self.transpose_checkbox.setChecked(False)
            root_layout.addWidget(self.transpose_checkbox)

            actions_row = QHBoxLayout()
            self.export_button = QPushButton("Export and save")
            self.copy_button = QPushButton("Copy output")
            self.copy_button.setEnabled(False)
            actions_row.addWidget(self.export_button)
            actions_row.addWidget(self.copy_button)
            root_layout.addLayout(actions_row)

            self.status_label = QLabel("Select a font BMP file and click Export and save.")
            root_layout.addWidget(self.status_label)

            self.summary_label = QLabel("Summary: -")
            root_layout.addWidget(self.summary_label)

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

            self.input_browse_button.clicked.connect(self.select_input_file)
            self.input_edit.editingFinished.connect(self.try_preview_from_input_path)
            self.output_browse_button.clicked.connect(self.select_output_file)
            self.export_button.clicked.connect(self.export_file)
            self.copy_button.clicked.connect(self.copy_output)

            if DEFAULT_INPUT_PATH.exists():
                self.update_image_preview(DEFAULT_INPUT_PATH)

        def select_input_file(self) -> None:
            selected_file, _ = QFileDialog.getOpenFileName(
                self,
                "Select font BMP file",
                str(ASSETS_DIR),
                "Bitmap files (*.bmp);;All files (*.*)",
            )
            if not selected_file:
                return

            input_path = Path(selected_file)
            self.input_edit.setText(str(input_path))
            self.update_image_preview(input_path)

        def try_preview_from_input_path(self) -> None:
            input_value = self.input_edit.text().strip()
            if not input_value:
                self.clear_image_preview("Image preview: no file selected.")
                return

            input_path = Path(input_value)
            if input_path.exists() and input_path.is_file():
                self.update_image_preview(input_path)
                return

            self.clear_image_preview("Image preview: file not found.")

        def select_output_file(self) -> None:
            default_output = self.output_edit.text().strip() or str(DEFAULT_OUTPUT_PATH)
            selected_file, _ = QFileDialog.getSaveFileName(
                self,
                "Select output file",
                default_output,
                "Text files (*.txt);;C files (*.c);;All files (*.*)",
            )
            if selected_file:
                self.output_edit.setText(selected_file)

        def export_file(self) -> None:
            input_text = self.input_edit.text().strip()
            output_text = self.output_edit.text().strip()
            if not input_text:
                QMessageBox.warning(self, "Missing input", "Choose a font BMP file first.")
                return
            if not output_text:
                QMessageBox.warning(self, "Missing output", "Set output file path.")
                return

            input_path = Path(input_text)
            if not input_path.exists():
                QMessageBox.warning(self, "Missing input file", f"File not found: {input_path}")
                return

            output_path = Path(output_text)
            glyph_width = self.glyph_width_spin.value()
            glyph_height = self.glyph_height_spin.value()

            try:
                img_x, img_y, glyph_count, exported_text = convert_and_save(
                    input_path=input_path,
                    output_path=output_path,
                    glyph_width=glyph_width,
                    glyph_height=glyph_height,
                    transpose_glyphs=self.transpose_checkbox.isChecked(),
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
                f"image={img_x}x{img_y}, "
                f"glyphs={glyph_count}, "
                f"pixels={img_x * img_y}"
            )
            self.update_image_preview(input_path)

        def copy_output(self) -> None:
            if not self.last_output:
                return
            QApplication.clipboard().setText(self.last_output)
            self.status_label.setText("Output copied to clipboard.")

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
    window = FontExporterWindow()
    window.show()
    return app.exec()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert a BMP font atlas to packed RGB565 font data."
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
        "--glyph-width",
        type=int,
        default=DEFAULT_GLYPH_WIDTH,
        help=f"Glyph width in pixels (default: {DEFAULT_GLYPH_WIDTH}).",
    )
    parser.add_argument(
        "--glyph-height",
        type=int,
        default=DEFAULT_GLYPH_HEIGHT,
        help=f"Glyph height in pixels (default: {DEFAULT_GLYPH_HEIGHT}).",
    )
    parser.add_argument(
        "--transpose",
        action="store_true",
        help="Transpose each glyph before exporting.",
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

    try:
        img_x, img_y, glyph_count, output_text = convert_and_save(
            input_path=input_path,
            output_path=args.output,
            glyph_width=args.glyph_width,
            glyph_height=args.glyph_height,
            transpose_glyphs=args.transpose,
        )
    except Exception as exc:
        print(f"Export failed: {exc}", file=sys.stderr)
        return 1

    print(f"Font atlas: {img_x}x{img_y} ({img_x * img_y} pixels)")
    print(f"Glyphs: {glyph_count}")
    print(f"Output: {args.output}")

    if args.stdout:
        print(output_text)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
