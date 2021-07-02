<?php

$plugin = array(
  'title' => 'Table and Other Displays for DB Query Results',
  'description' => 'Display database results in plain text, simple table, json, ul, ol, etc.',
  'single' => TRUE,
  'render callback' => 'table_display_db_content_type_render',
  'edit form' => 'table_display_db_content_type_edit_form',
  'category' => 'Gang',
  'all contexts' => TRUE,
  'admin title' => 'table_display_db_admin_title',
);

/**
 * Ctools edit form.
 */
function table_display_db_content_type_edit_form($form, &$form_state) {
  $conf = $form_state['conf'];
  global $databases;
  $theme_options = array('plain' => 'plain', 'table' => 'table', 'json' => 'json', 'item_list_ul' => 'item list (ul)', 'item_list_ol' => 'item list (ol)', 'item_list_ul_1st' => 'item list (ul) 1st', 'item_list_ul_1st2' => 'item list (ul) 1st5', 'restful_json' => 'Restful json output', );

  $form['admin_title'] = array(
    '#type' => 'textfield',
    '#title' => 'Administrative title',
    '#default_value' => $conf['admin_title'],
    '#weight' => -1000,
  );
  $form['db'] = array(
    '#type' => 'select',
    '#title' => t('Database'),
    '#description' => t('Select the databases from which to allow fetching the data.'),
    '#options' => array_combine(array_keys($databases), array_keys($databases)),
    '#default_value' => $conf['db'],
    '#required' => TRUE,
  );
  $form['header'] = array(
    '#type' => 'textarea',
    '#title' => t('Table Header'),
    '#description' => t('Enter the table header sequentially separated by comma, e.g. ID,Event,Event Time. Token_replace and ctools_context_keyword_substitute available.'),
    '#default_value' => $conf['header'],
    '#rows' => 2,
  );
  $form['query'] = array(
    '#type' => 'textarea',
    '#title' => t('SQL query'),
    '#description' => t('Enter the SQL query which will fetch the data to be imported. Do not use double quote. Use single quote. Token_replace and ctools_context_keyword_substitute available.<br>Use <span style="color:blue">:1, :2, :3 ...</span> as placeholders, starting with :1. They will be replaced by parameters below<br><span style="color:red">SUPER CAUTION: Do not give access to any others, and check sql code for SQL Injection!</span>'),
    '#default_value' => $conf['query'],
    '#required' => TRUE,
  );
  $form['parameter'] = array(
    '#type' => 'textfield',
    '#title' => t('Parameters!Default'),
    '#description' => t('Enter the parameters sequentially separated by comma, will replace to the placeholders in the query sequentially e.g. [token1],[token2]. Token_replace and ctools_context_keyword_substitute available. If use default when parameters is empty, use "!Default"'),
    '#default_value' => $conf['parameter'],
  );
  if (module_exists('token')) {
    $form['token_help'] = array(
      '#title' => t('Replacement patterns'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['token_help']['help'] = array(
      '#theme' => 'token_tree',
      '#token_types' => array(),
    );
  }
  $form['limit'] = array(
    '#type' => 'numberfield',
    '#title' => t('Pager Limit'),
    '#description' => t('Default to 1000.'),
    '#default_value' => isset($conf['limit']) ? $conf['limit'] : 1000,
  );
  $form['count_query'] = array(
    '#type' => 'textarea',
    '#title' => t('Count query'),
    '#description' => t('Count query for pager, count(*) AS total'),
    '#default_value' => $conf['count_query'],
    '#rows' => 2,
  );
  $form['theme'] = array(
    '#type' => 'select',
    '#title' => t('Choose the theme'),
    '#description' => t('item_list_ul_1st use 1st value as data, the rest as class attribute.'),
    '#default_value' => isset($conf['theme']) ? $conf['theme'] : 'table',
    '#options' => $theme_options,
  );

  return $form;
}

/**
 * Ctools edit form submit handler.
 */
function table_display_db_content_type_edit_form_submit($form, &$form_state) {
  $form_state['conf']['db'] = $form_state['values']['db'];
  $form_state['conf']['header'] = $form_state['values']['header'];
  $form_state['conf']['query'] = $form_state['values']['query'];
  $form_state['conf']['parameter'] = $form_state['values']['parameter'];
  $form_state['conf']['limit'] = $form_state['values']['limit'];
  $form_state['conf']['count_query'] = $form_state['values']['count_query'];
  $form_state['conf']['theme'] = $form_state['values']['theme'];
  $form_state['conf']['admin_title'] = $form_state['values']['admin_title'];
}

/**
 * Implements hook_admin_title().
 */
function table_display_db_admin_title($subtype, $conf, $context = NULL) {
  $title = empty($conf['admin_title']) ? '' : ': ' . $conf['admin_title'];
  return t('Table Display DB: ' . $title);
}

/**
 * Render callback function.
 */
function table_display_db_content_type_render($subtype, $conf, $args, $context) {
  //Get header
  $head = array_map('trim', explode(',', ($conf['header'])));
  foreach ($head as $key => $value) {
    $header[] = token_replace(ctools_context_keyword_substitute($value, array(), $context), array(), array('clear' => TRUE));
  }
  $parameter = array_map('trim', explode(',', ($conf['parameter'])));
  $query = trim($conf['query']);
  // Make sure there is only one query
  if ($end = strpos($query, ';')) $query = substr($query, 0, $end + 1);
  //$query = token_replace(ctools_context_keyword_substitute($query, array(), $context), array(), array('clear' => TRUE));
  $count_query = trim($conf['count_query']);
  if ($end = strpos($count_query, ';')) $count_query = substr($count_query, 0, $end + 1);

  // replace placeholder, default value
  foreach ($parameter as $key => $value) {
    $value = explode("!", $value);
    $parameters[':'.($key+1)] = token_replace(ctools_context_keyword_substitute($value[0], array(), $context), array(), array('clear' => TRUE));
    if (empty($parameters[':'.($key+1)])) $parameters[':'.($key+1)] = token_replace(ctools_context_keyword_substitute($value[1], array(), $context), array(), array('clear' => TRUE));
  }

  // Run the query on the selected database
  $db = $conf['db'];
  try {
    db_set_active($db);
    //$count = 0; // prevent large results
    $limit = isset($conf['limit']) ? $conf['limit'] : 1000;
    $start = isset($_REQUEST['page']) ? $_REQUEST['page'] * $limit : 0;
    $result = @db_query_range($query,$start,$limit,$parameters);
    foreach ($result as $record) {
      $results[] = (array)$record;
    }
    if ($count_query) {
      $count_result = @db_query($count_query);
      foreach ($count_result as $count) {
        $counts[] = $count; //get the count of rows
      }
    }
    // Switch back to the default database
    db_set_active();
  }
  catch (Exception $error) {
    form_set_error('query', $error->getMessage());
    db_set_active();
  }
  
  $items = array();
  switch ($conf['theme']) {
    case 'table':
      $output = theme('table', array(
        'header' => $header,
        'rows' => $results,
      ));
      if ($counts) {
        pager_default_initialize($counts[0]->total, $limit, $element = 0); // must use count(*) AS total
        $output = theme('pager',array("quantity"=>5)).$output.theme('pager',array("quantity"=>5));
      }
      break;
    case 'json':
      $output = '<div class="json-tddb-output">'.json_encode($results).'</div>';
      break;
    case 'item_list_ul':
      foreach ($results as $key => $result) {
        $items[] = array('data' => implode(', ', $result), );
      }
      $output = theme('item_list', array(
        'type' => 'ul',
        'items' => $items,
      ));
      break;
    case 'item_list_ol':
      foreach ($results as $key => $result) {
        $items[] = array('data' => implode(', ', $result), );
      }
      $output = theme('item_list', array(
        'type' => 'ol',
        'items' => $items,
      ));
      break;
    case 'item_list_ul_1st': //extract 1st col as text, the rest cols as class
      foreach ($results as $key => $result) {
        $value = array_shift($result);
        $items[] = array(
          'data' => $value, 
          'class' => array_values($result), 
        );
      }
      $output = theme('item_list', array(
        'type' => 'ul',
        'items' => $items,
      ));
      break;
    case 'item_list_ul_1st2': //extract 1st col as text, 2nd as title, 3rd a3, 4th a4, 5th a5, the rest cols as class
      foreach ($results as $key => $result) {
        $value = array_shift($result);
        $title = array_shift($result);
        $a3 = array_shift($result);
        $a4 = array_shift($result);
        $a5 = array_shift($result);
        $items[] = array(
          'data' => $value, 
          $header[0] => $title, 
          $header[1] => $a3, 
          $header[2] => $a4, 
          $header[3] => $a5, 
          'class' => array_values($result), 
        );
      }
      $output = theme('item_list', array(
        'type' => 'ul',
        'items' => $items,
      ));
      break;
    case 'restful_json':
      drupal_json_output(array(
          'data' => $results,
      ));
      drupal_exit();
    case 'plain': default:
      foreach ($results as $key => $result) {
        $items[] = implode(',', $result);
      }
      $output = implode('|', $items);
      break;
  } //dpm($results);dpm($query);
  $block = new stdClass();
  $block->content = $output;
  return $block;
}
