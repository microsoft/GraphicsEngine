"""Utilities for working with Google's Gemini 2.5 models for image understanding and generation."""

from __future__ import annotations

import base64
import json

import mimetypes
import os
from pathlib import Path
from typing import Any, Dict, Optional

try:
    from google import genai  # type: ignore
except ImportError as exc:  # pragma: no cover - library import guard
    raise ImportError(
        "The google-genai package is required. Install it via `pip install google-genai`."
    ) from exc

DEFAULT_VISION_MODEL = "gemini-2.0-flash-lite"
DEFAULT_IMAGE_MODEL = "imagen-4.0-generate-001"
_SUPPORTED_ASPECT_RATIOS = {
    "1:1": 1.0,
    "3:4": 0.75,
    "4:3": 4 / 3,
    "9:16": 9 / 16,
    "16:9": 16 / 9,
}

__all__ = ["describe_image", "generate_image_from_text"]


def _resolve_api_key(api_key: Optional[str]) -> str:
    """Return the API key to use for Google Gemini requests."""
    key = api_key or os.getenv("GOOGLE_API_KEY")
    if not key:
        raise ValueError(
            "A Google Gemini API key is required. Set GOOGLE_API_KEY or pass api_key explicitly."
        )
    return key


def _build_client(api_key: Optional[str]) -> "genai.Client":
    """Instantiate a Gemini client with the resolved API key."""
    return genai.Client(api_key=_resolve_api_key(api_key))


def _strip_code_fences(text: str) -> str:
    """Remove Markdown code fences from a Gemini response if they are present."""
    stripped = text.strip()
    if stripped.startswith("```"):
        lines = stripped.splitlines()
        if len(lines) >= 3 and lines[0].startswith("```") and lines[-1] == "```":
            return "\n".join(lines[1:-1]).strip()
    return stripped


def _extract_text_from_response(response: Any) -> str:
    """Pull the textual content out of a Gemini response object."""
    text = getattr(response, "text", None)
    if isinstance(text, str) and text.strip():
        return text.strip()

    candidates = getattr(response, "candidates", None)
    parts: list[str] = []
    if candidates:
        for candidate in candidates:
            candidate_content = getattr(candidate, "content", None)
            candidate_parts = []
            if candidate_content is not None and hasattr(candidate_content, "parts"):
                candidate_parts = candidate_content.parts
            elif hasattr(candidate, "parts"):
                candidate_parts = candidate.parts  # type: ignore[attr-defined]

            for part in candidate_parts or []:
                part_text = getattr(part, "text", None)
                if isinstance(part_text, str):
                    parts.append(part_text)

    if parts:
        return "\n".join(piece.strip() for piece in parts if piece.strip())

    return ""


def _extract_image_bytes(response: Any) -> bytes:
    """Pull binary image data out of a Gemini response object."""
    generated_images = getattr(response, "generated_images", None)
    if generated_images:
        for item in generated_images:
            image = getattr(item, "image", None)
            if image is not None:
                data = getattr(image, "image_bytes", None) or getattr(image, "data", None)
                if data:
                    return data if isinstance(data, bytes) else base64.b64decode(data)

    images = getattr(response, "images", None)
    if images:
        for image in images:
            if image is None:
                continue
            data = getattr(image, "image_bytes", None) or getattr(image, "data", None)
            if data:
                return data if isinstance(data, bytes) else base64.b64decode(data)

    binary_outputs = getattr(response, "binary_outputs", None)
    if binary_outputs:
        for item in binary_outputs:
            data = getattr(item, "data", None) or getattr(item, "image_bytes", None)
            if data:
                return data if isinstance(data, bytes) else base64.b64decode(data)

    content = getattr(response, "content", None)
    parts = getattr(content, "parts", None) if content else None
    if parts:
        for part in parts:
            inline_data = getattr(part, "inline_data", None)
            if inline_data:
                data = getattr(inline_data, "data", None)
                if data:
                    return data if isinstance(data, bytes) else base64.b64decode(data)

    candidates = getattr(response, "candidates", None)
    if candidates:
        for candidate in candidates:
            candidate_content = getattr(candidate, "content", None)
            candidate_parts = getattr(candidate_content, "parts", None) if candidate_content else None
            if candidate_parts:
                for part in candidate_parts:
                    inline_data = getattr(part, "inline_data", None)
                    if inline_data:
                        data = getattr(inline_data, "data", None)
                        if data:
                            return data if isinstance(data, bytes) else base64.b64decode(data)

    raise RuntimeError("Gemini response did not include image bytes.")


def _derive_aspect_ratio(width: int, height: int) -> str:
    """Select the closest supported aspect ratio for the requested dimensions."""
    ratio = width / height
    return min(
        _SUPPORTED_ASPECT_RATIOS.items(),
        key=lambda item: abs(item[1] - ratio),
    )[0]


def _derive_image_size(width: int, height: int) -> str:
    """Translate absolute dimensions into the SDK's size keyword."""
    largest = max(width, height)
    return "1K" if largest <= 1024 else "2K"


def _build_image_generation_config(
    width: int,
    height: int,
    *,
    aspect_ratio: Optional[str] = None,
    image_size: Optional[str] = None,
    guidance_scale: Optional[float] = None,
    negative_prompt: Optional[str] = None,
) -> Dict[str, Any]:
    """Create a config payload acceptable by client.models.generate_images."""
    config: Dict[str, Any] = {"number_of_images": 1}
    config["aspect_ratio"] = aspect_ratio or _derive_aspect_ratio(width, height)
    config["image_size"] = image_size or _derive_image_size(width, height)
    if guidance_scale is not None:
        config["guidance_scale"] = guidance_scale
    if negative_prompt:
        config["negative_prompt"] = negative_prompt
    return config


def describe_image(
    image_path: str,
    *,
    api_key: Optional[str] = None,
    model: str = DEFAULT_VISION_MODEL,
    temperature: float = 0.2,
    is_texture: bool = False,
) -> Dict[str, str]:
    """Generate a rich description of an image using Gemini 2.5.

    Args:
        image_path: Path to the image to analyse.
        api_key: Optional override for the Gemini API key.
        model: Vision-capable Gemini model to use.
        temperature: Sampling temperature for the generation.
        is_texture: Whether this is a texture file for 3D graphics.

    Returns:
        A dictionary containing summary, caption, description, and sentiment fields.
    """
    path = Path(image_path)
    if not path.is_file():
        raise FileNotFoundError(f"Image not found: {path}")

    mime_type, _ = mimetypes.guess_type(path.name)
    if not mime_type:
        raise ValueError(f"Unable to determine MIME type for {path}")

    image_bytes = path.read_bytes()
    client = _build_client(api_key)

    if is_texture:
        prompt = (
            "You are an assistant that analyses texture images for 3D graphics and returns structured data. "
            "Look at the provided TEXTURE image and respond with a single JSON object containing the keys "
            "summary, caption, description, and sentiment. Focus on the surface material properties and patterns. "
            "- summary: one sentence (<= 30 words) describing the texture type and surface properties. "
            "- caption: a short caption (<= 12 words) describing the material/surface type. "
            "- description: two sentences highlighting texture details, patterns, and material characteristics suitable for 3D rendering. "
            "- sentiment: one of ['positive', 'neutral', 'negative'] representing the overall visual appeal. "
            "Return only valid JSON without commentary or code fences."
        )
    else:
        prompt = (
            "You are an assistant that analyses images and returns structured data. "
            "Look at the provided image and respond with a single JSON object containing the keys "
            "summary, caption, description, and sentiment. "
            "- summary: one sentence (<= 30 words) describing the overall scene. "
            "- caption: a short caption (<= 12 words) suitable for social media. "
            "- description: two sentences highlighting notable details. "
            "- sentiment: one of ['positive', 'neutral', 'negative'] representing the dominant mood. "
            "Return only valid JSON without commentary or code fences."
        )

    response = client.models.generate_content(
        model=model,
        contents=[
            {
                "role": "user",
                "parts": [
                    {"text": prompt},
                    {
                        "inline_data": {
                            "mime_type": mime_type,
                            "data": base64.b64encode(image_bytes).decode("utf-8"),
                        }
                    },
                ],
            }
        ],
        config={"temperature": temperature},
    )

    text = _strip_code_fences(_extract_text_from_response(response))
    if not text:
        raise RuntimeError("Gemini did not return a textual description for the image.")

    try:
        payload = json.loads(text)
    except json.JSONDecodeError as exc:
        raise ValueError(f"Gemini returned non-JSON output: {text}") from exc

    return {
        "summary": str(payload.get("summary", "")).strip(),
        "caption": str(payload.get("caption", "")).strip(),
        "description": str(payload.get("description", "")).strip(),
        "sentiment": str(payload.get("sentiment", "")).strip().lower(),
    }


def generate_image_from_text(
    prompt: str,
    width: int,
    height: int,
    output_path: str,
    *,
    api_key: Optional[str] = None,
    model: str = DEFAULT_IMAGE_MODEL,
    aspect_ratio: Optional[str] = None,
    image_size: Optional[str] = None,
    guidance_scale: Optional[float] = None,
    negative_prompt: Optional[str] = None,
    is_texture: bool = False,
) -> Path:
    """Create an image with Gemini 2.5 from a text prompt.

    Args:
        prompt: Natural language description of the scene to render.
        width: Desired image width in pixels. Determines the aspect ratio selection.
        height: Desired image height in pixels. Determines the aspect ratio selection.
        output_path: File path where the generated image should be saved.
        api_key: Optional override for the Gemini API key.
        model: Image-capable Gemini model to use.
        aspect_ratio: Optional override. Supported values include 1:1, 3:4, 4:3, 9:16, 16:9.
        image_size: Optional override for the largest dimension ("1K" or "2K").
        guidance_scale: Optional guidance scale controlling prompt adherence.
        negative_prompt: Text describing concepts to avoid.
        is_texture: Whether to generate a seamless texture for 3D graphics.

    Returns:
        The path to the saved image file.
    """
    if width <= 0 or height <= 0:
        raise ValueError("Image dimensions must be positive integers.")

    client = _build_client(api_key)
    config = _build_image_generation_config(
        width,
        height,
        aspect_ratio=aspect_ratio,
        image_size=image_size,
        guidance_scale=guidance_scale,
        negative_prompt=negative_prompt,
    )

    # Enhance prompt for texture generation
    if is_texture:
        enhanced_prompt = (
            f"Generate a seamless, tileable texture for 3D graphics: {prompt}. "
            "The texture should be high-quality, uniform, and suitable for repeating across surfaces. "
            "Focus on material properties and surface details that work well in 3D rendering. "
            "Avoid seams, borders, edges, and ensure the texture tiles seamlessly."
        )
        # Remove negative_prompt from config for texture generation since Gemini doesn't support it
        if "negative_prompt" in config:
            del config["negative_prompt"]
    else:
        enhanced_prompt = prompt

    try:
        response = client.models.generate_images(
            model=model,
            prompt=enhanced_prompt,
            config=config,
        )
    except AttributeError as exc:
        raise RuntimeError(
            "The installed google-genai client does not expose models.generate_images. "
            "Update the SDK to a version that supports image generation with Gemini 2.5."
        ) from exc

    if not getattr(response, "generated_images", None) and not getattr(response, "images", None):
        raise RuntimeError("Gemini did not return any image data.")

    image_bytes = _extract_image_bytes(response)
    if isinstance(image_bytes, str):
        image_bytes = base64.b64decode(image_bytes)

    output_file = Path(output_path)
    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_bytes(image_bytes)
    return output_file
