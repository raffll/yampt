import ctranslate2
import os
from huggingface_hub import hf_hub_download

models = [
    ("en-de", "Helsinki-NLP/opus-mt-en-de"),
    ("en-pl", "Helsinki-NLP/opus-mt-en-zlw"),
    ("en-fr", "Helsinki-NLP/opus-mt-en-fr"),
]

for lang_pair, repo in models:
    base_dir = f"models/{lang_pair}"
    model_dir = f"{base_dir}/model"

    os.makedirs(base_dir, exist_ok=True)

    print(f"Converting {lang_pair} from {repo}...")
    converter = ctranslate2.converters.TransformersConverter(repo)
    converter.convert(model_dir, quantization="int8", force=True)

    print(f"Downloading SentencePiece vocabs...")
    hf_hub_download(repo, "source.spm", local_dir=base_dir)
    hf_hub_download(repo, "target.spm", local_dir=base_dir)

    print(f"{lang_pair} done.\n")

print("All models downloaded and converted.")
