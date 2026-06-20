"""
Fine-tune NLLB-600M on Morrowind EN->PL translation pairs using LoRA.
Merges the adapter and converts to CTranslate2 int8 for use in yampt.translator.

Prerequisites:
    pip install torch transformers peft datasets sentencepiece accelerate ctranslate2

Usage:
    python finetune_model.py

Input:  JSON dict files from tests/pl/ (or specify DICT_DIR below)
Output: models/nllb-600M/model/ (overwritten with fine-tuned int8 model)
"""

import json
import os
import sys

DICT_DIR = "tests/pl"
MODEL_NAME = "facebook/nllb-200-distilled-600M"
OUTPUT_DIR = "models/nllb-600M/model"
LORA_DIR = "./finetune_tmp/lora"
MERGED_DIR = "./finetune_tmp/merged"
SRC_LANG = "eng_Latn"
TGT_LANG = "pol_Latn"
MIN_LENGTH = 5
MAX_LENGTH = 128
EPOCHS = 3
BATCH_SIZE = 16
LEARNING_RATE = 3e-4


def extract_pairs():
    pairs = []

    for fname in os.listdir(DICT_DIR):
        if not fname.endswith(".json"):
            continue

        path = os.path.join(DICT_DIR, fname)
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)

        for key, chapter in data.items():
            if not isinstance(chapter, list):
                continue

            for entry in chapter:
                old = entry.get("old", "").strip()
                new = entry.get("new", "").strip()

                if not old or not new:
                    continue

                if old == new:
                    continue

                if len(old) < MIN_LENGTH:
                    continue

                pairs.append({"source": old, "target": new})

    print(f"Extracted {len(pairs)} training pairs from {DICT_DIR}")
    return pairs


def train(pairs):
    from datasets import Dataset
    from transformers import (
        AutoModelForSeq2SeqLM,
        AutoTokenizer,
        Seq2SeqTrainingArguments,
        Seq2SeqTrainer,
    )
    from peft import get_peft_model, LoraConfig, TaskType

    print(f"Loading model: {MODEL_NAME}")
    tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
    model = AutoModelForSeq2SeqLM.from_pretrained(MODEL_NAME)

    lora_config = LoraConfig(
        task_type=TaskType.SEQ_2_SEQ_LM,
        r=16,
        lora_alpha=32,
        lora_dropout=0.05,
        target_modules=["q_proj", "v_proj", "k_proj", "out_proj"],
    )
    model = get_peft_model(model, lora_config)
    model.print_trainable_parameters()

    def preprocess(examples):
        tokenizer.src_lang = SRC_LANG
        inputs = tokenizer(
            examples["source"],
            max_length=MAX_LENGTH,
            truncation=True,
            padding="max_length",
        )
        tokenizer.src_lang = TGT_LANG
        targets = tokenizer(
            examples["target"],
            max_length=MAX_LENGTH,
            truncation=True,
            padding="max_length",
        )
        inputs["labels"] = targets["input_ids"]
        return inputs

    dataset = Dataset.from_list(pairs)
    dataset = dataset.map(preprocess, batched=True, remove_columns=["source", "target"])
    split = dataset.train_test_split(test_size=0.05, seed=42)

    print(f"Training: {len(split['train'])} samples, eval: {len(split['test'])} samples")

    import torch
    use_fp16 = torch.cuda.is_available()

    args = Seq2SeqTrainingArguments(
        output_dir=LORA_DIR,
        num_train_epochs=EPOCHS,
        per_device_train_batch_size=BATCH_SIZE,
        per_device_eval_batch_size=BATCH_SIZE,
        learning_rate=LEARNING_RATE,
        weight_decay=0.01,
        eval_strategy="epoch",
        save_strategy="epoch",
        logging_steps=50,
        fp16=use_fp16,
        predict_with_generate=False,
        report_to="none",
    )

    trainer = Seq2SeqTrainer(
        model=model,
        args=args,
        train_dataset=split["train"],
        eval_dataset=split["test"],
        tokenizer=tokenizer,
    )

    print("Starting training...")
    trainer.train()

    model.save_pretrained(LORA_DIR)
    tokenizer.save_pretrained(LORA_DIR)
    print(f"LoRA adapter saved to {LORA_DIR}")


def merge_and_convert():
    from transformers import AutoModelForSeq2SeqLM, AutoTokenizer
    from peft import PeftModel
    import ctranslate2

    print("Merging LoRA weights into base model...")
    base_model = AutoModelForSeq2SeqLM.from_pretrained(MODEL_NAME)
    model = PeftModel.from_pretrained(base_model, LORA_DIR)
    merged = model.merge_and_unload()

    os.makedirs(MERGED_DIR, exist_ok=True)
    merged.save_pretrained(MERGED_DIR)

    tokenizer = AutoTokenizer.from_pretrained(LORA_DIR)
    tokenizer.save_pretrained(MERGED_DIR)
    print(f"Merged model saved to {MERGED_DIR}")

    print(f"Converting to CTranslate2 int8 -> {OUTPUT_DIR}")
    converter = ctranslate2.converters.TransformersConverter(MERGED_DIR)
    converter.convert(OUTPUT_DIR, quantization="int8", force=True)
    print("Conversion complete")


def cleanup():
    import shutil

    if os.path.isdir("./finetune_tmp"):
        shutil.rmtree("./finetune_tmp")
        print("Cleaned up temporary files")


def main():
    if not os.path.isdir(DICT_DIR):
        print(f"Error: dictionary directory not found: {DICT_DIR}")
        print("Run integration tests first to generate base dictionaries, or change DICT_DIR.")
        sys.exit(1)

    pairs = extract_pairs()
    if len(pairs) < 100:
        print(f"Warning: only {len(pairs)} pairs found. Results may be poor with < 1000 pairs.")

    train(pairs)
    merge_and_convert()
    cleanup()

    print(f"\nDone. Fine-tuned model is at: {OUTPUT_DIR}")
    print("Restart yampt.translator to use the updated model.")


if __name__ == "__main__":
    main()
