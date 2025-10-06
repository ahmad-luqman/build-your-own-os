#!/bin/bash

# Delete old GitHub project

echo "=== Deleting Old Project #8 ==="
echo ""

# Delete project #8
echo "Deleting project #8..."
echo "This will delete project #8. The command is interactive."
gh project delete 8 --owner ahmad-luqman || echo "Project may already be deleted or you need to delete it manually"

echo ""
echo "✅ Old project cleanup complete"
echo ""
echo "Your new project #9 is ready at: https://github.com/users/ahmad-luqman/projects/9"