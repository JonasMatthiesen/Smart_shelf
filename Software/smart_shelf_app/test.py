import json
from pprint import pprint  # Pretty-print the output
from API import fetch_shelf_data  # Replace 'your_module' with actual module name

# 🔹 Test Smart Shelf IDs
test_shelf_ids = ["smart_shelf_1", "smart_shelf_2"]

def test_fetch_shelf_data():
    """Test fetching and displaying smart shelf data."""
    for smart_shelf_id in test_shelf_ids:
        print(f"\n🔍 Fetching data for: {smart_shelf_id}")
        shelf_data = fetch_shelf_data(smart_shelf_id)

        if shelf_data:
            print("\n📦 Structured Smart Shelf Data:")
            pprint(shelf_data, sort_dicts=False)  # Pretty-print the structured JSON
        else:
            print(f"⚠️ No data returned for {smart_shelf_id}")

if __name__ == "__main__":
    test_fetch_shelf_data()
