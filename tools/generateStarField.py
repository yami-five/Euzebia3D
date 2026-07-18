from __future__ import annotations

import argparse
import math
import random
import re
import sys
from collections.abc import Sequence


DEFAULT_STAR_COUNT = 1024
DEFAULT_SHIFT = 12
DEFAULT_ARRAY_NAME = "starField"
DEFAULT_STAR_RGB = (255, 255, 255)
STAR_RADIUS = 100.0
MAX_FIXED_SHIFT = 24
MAX_STAR_COUNT = 1_000_000


def positive_star_count(value: str) -> int:
    count = int(value)
    if not 1 <= count <= MAX_STAR_COUNT:
        raise argparse.ArgumentTypeError(
            f"Star count must be between 1 and {MAX_STAR_COUNT}."
        )
    return count


def fixed_shift(value: str) -> int:
    shift = int(value)
    if not 0 <= shift <= MAX_FIXED_SHIFT:
        raise argparse.ArgumentTypeError(
            f"Fixed-point shift must be between 0 and {MAX_FIXED_SHIFT}."
        )
    return shift


def to_c_identifier(value: str) -> str:
    identifier = re.sub(r"[^0-9A-Za-z_]", "_", value.strip())
    if not identifier:
        return DEFAULT_ARRAY_NAME
    if identifier[0].isdigit():
        return f"_{identifier}"
    return identifier


def parse_rgb(value: str) -> tuple[int, int, int]:
    try:
        red, green, blue = (int(component.strip()) for component in value.split(","))
    except ValueError as exc:
        raise ValueError("Color must have the form R,G,B, for example 255,255,255.") from exc

    if not all(0 <= component <= 255 for component in (red, green, blue)):
        raise ValueError("Each RGB component must be between 0 and 255.")
    return red, green, blue


def rgb_to_rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def random_unit_quaternion(rng: random.Random) -> tuple[float, float, float, float]:
    """Return a uniformly distributed unit quaternion as (x, y, z, w)."""
    u1 = rng.random()
    u2 = rng.random()
    u3 = rng.random()
    root_one_minus_u1 = math.sqrt(1.0 - u1)
    root_u1 = math.sqrt(u1)
    angle_u2 = math.tau * u2
    angle_u3 = math.tau * u3
    return (
        root_one_minus_u1 * math.sin(angle_u2),
        root_one_minus_u1 * math.cos(angle_u2),
        root_u1 * math.sin(angle_u3),
        root_u1 * math.cos(angle_u3),
    )


def rotate_vector_by_quaternion(
    vector: tuple[float, float, float], quaternion: tuple[float, float, float, float]
) -> tuple[float, float, float]:
    """Rotate vector by a unit quaternion without constructing temporary quaternions."""
    x, y, z = vector
    qx, qy, qz, qw = quaternion

    twice_cross_x = 2.0 * (qy * z - qz * y)
    twice_cross_y = 2.0 * (qz * x - qx * z)
    twice_cross_z = 2.0 * (qx * y - qy * x)

    return (
        x + qw * twice_cross_x + (qy * twice_cross_z - qz * twice_cross_y),
        y + qw * twice_cross_y + (qz * twice_cross_x - qx * twice_cross_z),
        z + qw * twice_cross_z + (qx * twice_cross_y - qy * twice_cross_x),
    )


def float_to_fixed(value: float, shift: int) -> int:
    """Match the renderer's C conversion: (int32_t)(value * (1 << shift))."""
    return int(value * (1 << shift))


def generate_star_positions(
    count: int, shift: int, rng: random.Random
) -> list[tuple[int, int, int]]:
    source_point = (0.0, 0.0, STAR_RADIUS)
    return [
        tuple(
            float_to_fixed(component, shift)
            for component in rotate_vector_by_quaternion(
                source_point, random_unit_quaternion(rng)
            )
        )
        for _ in range(count)
    ]


def format_star_field(
    positions: Sequence[tuple[int, int, int]],
    array_name: str,
    shift: int,
    color_rgb: tuple[int, int, int],
) -> str:
    color_rgb565 = rgb_to_rgb565(*color_rgb)
    color_literal = f"0x{color_rgb565:04x}u"
    lines = [
        f"// {len(positions)} stars on a sphere with radius {STAR_RADIUS:.1f}, Q{shift} fixed point.",
        f"// Star color: RGB({color_rgb[0]}, {color_rgb[1]}, {color_rgb[2]}) = {color_literal} (RGB565).",
        f"static Point3D {array_name}[{len(positions)}] = {{",
    ]
    lines.extend(
        f"    {{.point = {{{x}, {y}, {z}}}, .color = {color_literal}}},"
        for x, y, z in positions
    )
    lines.append("};")
    return "\n".join(lines)


def generate_output(
    count: int,
    shift: int,
    array_name: str,
    color_rgb: tuple[int, int, int],
    seed: int | None,
) -> str:
    rng = random.Random(seed)
    positions = generate_star_positions(count=count, shift=shift, rng=rng)
    return format_star_field(
        positions=positions,
        array_name=to_c_identifier(array_name),
        shift=shift,
        color_rgb=color_rgb,
    )


def run_gui(initial_count: int, initial_shift: int, initial_seed: int | None) -> int:
    try:
        from PySide6.QtWidgets import (
            QApplication,
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

    class StarFieldGeneratorWindow(QMainWindow):
        def __init__(self) -> None:
            super().__init__()
            self.setWindowTitle("Star Field Generator")
            self.resize(900, 700)

            central_widget = QWidget()
            self.setCentralWidget(central_widget)
            layout = QVBoxLayout(central_widget)
            form = QFormLayout()
            layout.addLayout(form)

            self.count_input = QSpinBox()
            self.count_input.setRange(1, MAX_STAR_COUNT)
            self.count_input.setValue(initial_count)
            form.addRow("Stars:", self.count_input)

            self.shift_input = QSpinBox()
            self.shift_input.setRange(0, MAX_FIXED_SHIFT)
            self.shift_input.setValue(initial_shift)
            form.addRow("Fixed-point shift:", self.shift_input)

            self.seed_input = QLineEdit("" if initial_seed is None else str(initial_seed))
            self.seed_input.setPlaceholderText("Optional; use the same seed to reproduce output")
            form.addRow("Random seed:", self.seed_input)

            self.name_input = QLineEdit(DEFAULT_ARRAY_NAME)
            form.addRow("C array name:", self.name_input)

            self.color_input = QLineEdit(",".join(str(component) for component in DEFAULT_STAR_RGB))
            self.color_input.setPlaceholderText("R,G,B; for example 255,255,255")
            form.addRow("Star color (RGB):", self.color_input)

            actions_layout = QHBoxLayout()
            self.generate_button = QPushButton("Generate")
            self.copy_button = QPushButton("Copy generated code")
            actions_layout.addWidget(self.generate_button)
            actions_layout.addWidget(self.copy_button)
            layout.addLayout(actions_layout)

            self.status_label = QLabel()
            layout.addWidget(self.status_label)

            self.output = QPlainTextEdit()
            self.output.setReadOnly(True)
            layout.addWidget(self.output)

            self.generate_button.clicked.connect(self.generate)
            self.copy_button.clicked.connect(self.copy_output)
            self.generate()

        def generate(self) -> None:
            try:
                seed_text = self.seed_input.text().strip()
                seed = int(seed_text) if seed_text else None
                output = generate_output(
                    count=self.count_input.value(),
                    shift=self.shift_input.value(),
                    array_name=self.name_input.text(),
                    color_rgb=parse_rgb(self.color_input.text()),
                    seed=seed,
                )
            except ValueError as exc:
                QMessageBox.warning(self, "Invalid input", str(exc))
                return

            self.output.setPlainText(output)
            self.status_label.setText(
                f"Generated {self.count_input.value()} stars in Q{self.shift_input.value()} fixed point."
            )

        def copy_output(self) -> None:
            QApplication.clipboard().setText(self.output.toPlainText())
            self.status_label.setText("Generated code copied to clipboard.")

    app = QApplication(sys.argv)
    window = StarFieldGeneratorWindow()
    window.show()
    return app.exec()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate Point3D stars by rotating (0.0, 0.0, 100.0) with random quaternions."
    )
    parser.add_argument(
        "count",
        nargs="?",
        type=positive_star_count,
        help="Number of stars to generate. Omit to open the PySide6 UI.",
    )
    parser.add_argument(
        "--shift",
        type=fixed_shift,
        default=DEFAULT_SHIFT,
        help=f"Fixed-point fractional-bit shift (default: {DEFAULT_SHIFT}).",
    )
    parser.add_argument(
        "--name",
        default=DEFAULT_ARRAY_NAME,
        help=f"Generated C array name (default: {DEFAULT_ARRAY_NAME}).",
    )
    parser.add_argument(
        "--color",
        type=parse_rgb,
        default=DEFAULT_STAR_RGB,
        metavar="R,G,B",
        help="Star color encoded as RGB565 in the generated C code (default: 255,255,255).",
    )
    parser.add_argument(
        "--seed",
        type=int,
        help="Optional random seed for reproducible output.",
    )
    parser.add_argument(
        "--no-ui",
        action="store_true",
        help="Run in command-line mode and print generated C code to stdout.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.count is None and not args.no_ui:
        return run_gui(
            initial_count=DEFAULT_STAR_COUNT,
            initial_shift=args.shift,
            initial_seed=args.seed,
        )

    if args.count is None:
        print("Missing count in --no-ui mode.", file=sys.stderr)
        return 2

    print(
        generate_output(
            count=args.count,
            shift=args.shift,
            array_name=args.name,
            color_rgb=args.color,
            seed=args.seed,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
