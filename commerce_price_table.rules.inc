<?php

/**
 * Implements hook_rules_action_info().
 */
function commerce_price_table_rules_action_info() {
  $actions = array();

  $actions['commerce_price_table_set_price'] = array(
    'label' => t('Set the unit price to a table based price'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Line item'),
      ),
      'quantity' => array(
        'label' => t('Quantity'),
        'type' => 'decimal',
      ),
      'price_table' => array(
        'label' => t('Price table'),
        'type' => 'list<commerce_price_table>',
      ),
      'component_name' => array(
        'type' => 'text',
        'label' => t('Price component type'),
        'description' => t('Price components track changes to prices made during the price calculation process, and they are carried over from the unit price to the total price of a line item. When an order total is calculated, it combines all the components of every line item on the order. When the unit price is altered by this action, the selected type of price component will be added to its data array and reflected in the order total display when it is formatted with components showing. Defaults to base price, which displays as the order Subtotal.'),
        'options list' => 'commerce_line_item_price_component_options_list',
        'default value' => 'base_price',
      ),
    ),
    'group' => t('Commerce Price Table'),
  );

  $actions['commerce_price_table_sum_quantities'] = array(
    'label' => t('Sum quantities for products sharing a display'),
    'parameter' => array(
      'commerce_line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Line item'),
        'wrapped' => TRUE,
      ),
    ),
    'provides' => array(
      'combined_product_sum' => array(
        'label' => t('Combined product sum'),
        'type' => 'decimal',
      ),
    ),
    'group' => t('Commerce Price Table'),
  );

  return $actions;
}

/**
 * Rules callback: executes the "Replace the price for a price table" action.
 */
function commerce_price_table_set_price($line_item, $quantity, $price_table, $component_name) {
  // If the line item contains a product...
  if (in_array($line_item->type, commerce_product_line_item_types())) {
    // Load its referenced product.
    $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
    $product = $line_item_wrapper->commerce_product->value();

    // Bail now if the unit price is unset.
    $unit_price = commerce_price_wrapper_value($line_item_wrapper, 'commerce_unit_price');

    if (empty($unit_price)) {
      return;
    }

    // Fetch the table based price for the current product quantity.
    $table_price = commerce_price_table_get_amount_qty($product, $quantity, $price_table);

    // If we got a valid table price...
    if (!empty($table_price)) {
      // If the currency is different from the current currency, convert it.
      if ($unit_price['currency_code'] != $table_price['currency_code']) {
        $line_item_wrapper->commerce_unit_price->amount = commerce_currency_convert($unit_price['amount'], $unit_price['currency_code'], $table_price['currency_code']);
        $line_item_wrapper->commerce_unit_price->currency_code = $table_price['currency_code'];

        // Convert the currency code of the price's components.
        if (!empty($unit_price['data']['components'])) {
          foreach ($unit_price['data']['components'] as $key => &$component) {
            $component['price']['amount'] = commerce_currency_convert($component['price']['amount'], $component['price']['currency_code'], $table_price['currency_code']);
            $component['price']['currency_code'] = $table_price['currency_code'];
          }

          $wrapper->commerce_unit_price->data = $unit_price['data'];
        }
      }

      // Calculate the difference between the current unit price amount and the
      // table price and create a price array representing the difference.
      $current_amount = $unit_price['amount'];
      $updated_amount = $table_price['amount'];

      $difference = array(
        'amount' => $updated_amount - $current_amount,
        'currency_code' => $table_price['currency_code'],
        'data' => array(),
      );

      // Set the amount of the unit price and add the difference as a component.
      $line_item_wrapper->commerce_unit_price->amount = $updated_amount;

      $line_item_wrapper->commerce_unit_price->data = commerce_price_component_add(
        $line_item_wrapper->commerce_unit_price->value(),
        $component_name,
        $difference,
        TRUE
      );
    }
  }
}

/**
 * Rules callback: provides a combined product sum variable in a rule based on
 * all line items sharing the same product display as the given one.
 */
function commerce_price_table_sum_quantities($line_item_wrapper) {
  $sum = 0;

  // Loop over all line items on the order...
  foreach ($line_item_wrapper->order->commerce_line_items as $delta => $target_line_item_wrapper) {
    // If the current target line item is the same as the original one, include
    // its quantity in the sum.
    if ($line_item_wrapper->line_item_id->value() == $target_line_item_wrapper->line_item_id->value()) {
      $sum += $target_line_item_wrapper->quantity->value();
      continue;
    }

    // If the current target line item shares the same display context as the
    // original one, include its quantity in the sum.
    $source_data = $line_item_wrapper->value()->data['context'];
    $target_data = $target_line_item_wrapper->value()->data['context'];

    if ($source_data['product_ids'] == $target_data['product_ids']) {
      // Include special checking for product IDs derived from reference fields.
      if ($source_data['product_ids'] == 'entity') {
        if ($source_data['entity'] == $target_data['entity']) {
          $sum += $target_line_item_wrapper->quantity->value();
        }
      }
      else {
        $sum += $target_line_item_wrapper->quantity->value();
      }
    }
  }

  return array('combined_product_sum' => $sum);
}
