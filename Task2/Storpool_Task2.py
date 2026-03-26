from collections import Counter  # Counter helps count how many times each model appears
import bz2                       # bz2 lets us read .bz2 compressed files directly
import ijson                     # ijson parses JSON in a streaming way (without loading all into RAM)


def count_models(path: str):
    # counts[model] = how many times this model appears
    counts = Counter()

    # Stack to track nested JSON objects while streaming through the file.
    # Each object we enter gets its own small context dictionary.
    stack = []

    # These are JSON value types that can appear as field values.
    scalar_events = {"string", "number", "boolean", "null"}

    # Open the compressed file directly in binary read mode.
    # No need to manually decompress it first.
    with bz2.open(path, "rb") as f:

        # ijson.parse() reads the JSON token by token.
        # For each token it gives:
        # - prefix: where we are in the JSON structure
        # - event: what kind of token this is
        # - value: the actual value (if any)
        for prefix, event, value in ijson.parse(f):

            # A new JSON object starts: {...}
            if event == "start_map":
                # Create a small context for this object.
                # We only care about:
                # - current_key: the last field name seen
                # - model: the model value if found
                # - has_model: whether this object has a "model" field
                # - has_serial: whether this object has a "serial" field
                stack.append({
                    "current_key": None,
                    "model": None,
                    "has_model": False,
                    "has_serial": False,
                })

            # We found a key in a JSON object, for example "model" or "serial"
            elif event == "map_key":
                if stack:
                    stack[-1]["current_key"] = value

            # We found a scalar value such as a string/number/bool/null
            elif event in scalar_events:
                if not stack:
                    continue

                ctx = stack[-1]
                key = ctx["current_key"]

                # If the current field is "model", store its value
                if key == "model":
                    ctx["model"] = value
                    ctx["has_model"] = True

                # If the current field is "serial", just note that it exists
                elif key == "serial":
                    ctx["has_serial"] = True

            # End of current JSON object
            elif event == "end_map":
                ctx = stack.pop()

                # Count this object only if it contains BOTH:
                # - a model field
                # - a serial field
                #
                # This makes it likely that we are counting actual disk entries
                # instead of unrelated objects elsewhere in the JSON.
                if ctx["has_model"] and ctx["has_serial"]:
                    counts[str(ctx["model"])] += 1

    return counts


if __name__ == "__main__":
    # Path to the compressed input file
    path = "bigf.json.bz2"

    # Run the counting function
    counts = count_models(path)

    # Number of distinct models
    print(f"number_of_models={len(counts)}")

    # Print each model and how many times it appears
    # sorted() makes the output deterministic and easier to read
    for model in sorted(counts):
        print(f"{model}\t{counts[model]}")
